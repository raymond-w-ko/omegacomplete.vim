" script load guard
if exists('g:omegacomplete_loaded')
    finish
endif
let g:omegacomplete_loaded = 1

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" local script variables
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" this variable acts a guard to prevent needlessly sending the same
" buffer contents twice in a row
let s:just_did_insertenter = 0

" this variable indicates whether client side disambiguation mappings were
" performed
let s:performed_client_disambiguate_mappings = 0

" how many <C-p> to issue before <C-n> is issued so pressing ALT+number always
" goes to that selection
let s:last_disambiguate_index = -1

" path of log file to write debug output
" right now the only thing that uses this the stopwatch
if !exists('g:omegacomplete_log_file')
  let g:omegacomplete_log_file = ''
endif

" initialize this to be safe
let g:omegacomplete_server_results=[]

" current hilight mode (either normal or corrections)
let s:current_hi_mode = 'unitialized'

if !exists("g:omegacomplete_normal_hi_cmds")
    let g:omegacomplete_normal_hi_cmds=[
    \ "hi Pmenu guifg=#00ff00 guibg=#003300 gui=none " .
             \ "ctermbg=022 ctermfg=046 cterm=none",
    \ "hi PmenuSel guifg=#003300 guibg=#00ff00 gui=none " .
                \ "ctermbg=046 ctermfg=022 cterm=none",
        \ ]
endif
if !exists("g:omegacomplete_corrections_hi_cmds")
    let g:omegacomplete_corrections_hi_cmds=[
    \ "hi Pmenu guifg=#ffff00 guibg=#333300 gui=none " .
              \"ctermbg=058 ctermfg=226 cterm=none",
    \ "hi PmenuSel guifg=#333300 guibg=#ffff00 gui=none " .
                \ "ctermbg=226 ctermfg=058 cterm=none",
        \ ]
endif

if !exists("g:omegacomplete_autocomplete_suffix")
    let g:omegacomplete_autocomplete_suffix="jj"
endif

if !exists("g:omegacomplete_server_side_disambiguate")
    let g:omegacomplete_server_side_disambiguate=0
endif

if !exists("g:omegacomplete_client_side_disambiguate_mappings")
    let g:omegacomplete_client_side_disambiguate_mappings=0
endif

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" init
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function s:Init()
    if !s:loadCorePlugin()
        echom "failed to load omegacomplete core plugin"
        return
    endif

    set completefunc=OmegacompleteFunc

    " we pretty much have to force this otherwise this plugin is not helpful.
    set completeopt+=menu
    set completeopt+=menuone
    if v:version > 704 || (v:version == 704 && has('patch775'))
        set completeopt+=noselect
    endif

    command OmegacompleteFlushServerCaches :call <SID>FlushServerCaches()
    command OmegacompleteUpdateConfig :call <SID>UpdateConfig()
    command OmegacompleteDoTests :call <SID>DoTests()

    call <SID>UpdateConfig()

    augroup Omegacomplete
        autocmd!
    
        " whenever you delete a buffer, delete it from the server so that it
        " doesn't cause any outdated completions to be offered
        autocmd BufDelete * call <SID>OnBufDelete()
    
        " whenever we leave a current buffer, send it to the server to it can
        " decide if we have to update it. this keeps the state of the buffer
        " in sync so we can offer accurate completions when we switch to
        " another buffer
        autocmd BufLeave * call <SID>OnBufLeave()

        " just before you enter insert mode, send the contents of the buffer
        " so that the server has a chance to synchronize before we start
        " offering completions
        autocmd InsertEnter * call <SID>OnInsertEnter()

        " if we are idle, then we take this change to prune unused global
        " words set and trie. there are a variety of things which we consider
        " 'idle'
        autocmd CursorHold * call <SID>OnIdle()
        autocmd CursorHoldI * call <SID>OnIdle()
        autocmd FocusLost * call <SID>OnIdle()
        autocmd BufWritePre * call <SID>OnIdle()

        " when we open a file, send it contents to the server since we usually
        " have some time before we need to start editing the file (usually you
        " have to mentally parse where you are and/or go to the correct
        " location before entering insert mode)
        autocmd BufReadPost * call <SID>OnBufReadPost()

        " when the cursor has moved without the popup menu, we take this time
        " to send the buffer to the the completion engine. hopefully this
        " avoid the fragmented word problem.
        autocmd CursorMovedI * call <SID>OnCursorMovedI()
    augroup END
endfunction

function s:loadCorePlugin()
    python << EOF
import vim
import os

# find omegacomplete directory
path_list = vim.eval('&runtimepath').split(',')
omegacomplete_path = ""
for path in path_list:
    candidate_path = path + '/python/omegacomplete'
    if (os.path.exists(candidate_path)):
        omegacomplete_path = candidate_path

sys.path.append(omegacomplete_path)

# load omegacomplete python functions
client_path = omegacomplete_path + '/helper.py'
exec(compile(open(client_path).read(), client_path, 'exec'))
vim.command('let result = ' + oc_is_disabled())
EOF
    return result
endfunction

function s:UpdatePopupMenuColorscheme()
    if g:omegacomplete_is_corrections_only && s:current_hi_mode != 'corrections'
        for cmd in g:omegacomplete_corrections_hi_cmds
            exe cmd
        endfor
        let s:current_hi_mode = 'corrections'
    elseif !g:omegacomplete_is_corrections_only && s:current_hi_mode != 'normal'
        for cmd in g:omegacomplete_normal_hi_cmds
            exe cmd
        endfor
        let s:current_hi_mode = 'normal'
    endif
endfunction

function s:SendCurrentBuffer()
    python oc_send_current_buffer()
endfunction

function <SID>OnCursorMovedI()
    " disable when paste mode is active, to avoid massive processing lag
    if &paste
        return
    endif

    let line = getline('.')
    if len(line) == 0 
        return
    endif
    let cursor_ch = line[col('.') - 2]
    if cursor_ch != '-' && cursor_ch =~# '\W'
        return
    endif
    if pumvisible()
        return
    endif

    if v:version > 704 || (v:version == 704 && has('patch775'))
        call feedkeys("\<C-x>\<C-u>", 'n')
    else
        call feedkeys("\<C-x>\<C-u>\<C-p>", 'n')
    endif
endfunction

function <SID>PerformDisambiguate(index)
    let index = str2nr(a:index)
    if (index == 0)
        let lndex = 10
    endif

    if (index > len(g:omegacomplete_server_results))
        return ''
    endif

    let keys = ''
    if (s:last_disambiguate_index > 0)
        for i in range(s:last_disambiguate_index)
            let keys = keys . "\<C-p>"
        endfor
    endif
    for i in range(index)
        let keys = keys . "\<C-n>"
    endfor

    let s:last_disambiguate_index = index

    return keys
endfunction

" When we want to sync a buffer in normal mode. This mainly occurs when we
" are entering a buffer for the first time (i.e. it just opened), or we are
" switching windows and need it to be synced with the server in case you
" added or deleted lines
function <SID>NormalModeSyncBuffer()
    let buffer_number = bufnr('%')
    let buffer_name = bufname('%') 
    let absolute_path = escape(expand('%:p'), '\')

    " don't process these special buffers from other plugins
    if (match(buffer_name,
            \ '\v(GoToFile|ControlP|__Scratch__|__Gundo__|__Gundo_Preview__)') != -1)
        return
    endif

    exe 'py oc_eval("current_buffer_id ' . buffer_number . '")'
    exe 'py oc_eval("current_buffer_absolute_path ' . absolute_path . '")'
    call s:SendCurrentBuffer()
endfunction

" When we don't want a buffer loaded in memory in VIM, we can 'delete' the
" buffer. This must be reflected on the server, otherwise we get completions
" that we no longer want.
function <SID>OnBufDelete()
    let buffer_number = expand('<abuf>')
    exe 'py oc_eval("free_buffer ' . buffer_number . '")'

    call <SID>Prune()
endfunction

" Not that expensive, just checks for dead buffers and kills them if they are
" not in the set of alive buffers
function <SID>PruneBuffers()
    py oc_prune_buffers()
endfunction

" Fairly expensive as it has to rebuild it's list of words and shuffle them.
function <SID>Prune()
    py oc_prune()
endfunction

" When we are leaving buffer (either by opening another buffer, switching
" tabs, or moving to another split), then we have to inform send the contents
" of the buffer to the server since we could have changed the contents of the
" buffer while in normal though commands like 'dd'
function <SID>OnBufLeave()
    call <SID>NormalModeSyncBuffer()
endfunction

function <SID>OnInsertEnter()
    let s:just_did_insertenter = 1
    call <SID>NormalModeSyncBuffer()
    call <SID>PruneBuffers()
    call <SID>OnCursorMovedI()
endfunction

function <SID>OnIdle()
    call <SID>PruneBuffers()
    call <SID>Prune()
endfunction

function <SID>OnBufReadPost()
    call <SID>NormalModeSyncBuffer()
endfunction

" this needs to have global scope and it is what <C-X><C-U> depends on.
" don't think  it can use script / scope specific functions
function OmegacompleteFunc(findstart, base)
    python oc_update_current_buffer_info()

    " on first invocation a:findstart is 1 and base is empty
    if a:findstart
        python oc_compute_popup_list(False)
        " this should only be triggered if there is a valid initial match
        if len(g:omegacomplete_server_results) == 0
            if (v:version > 702)
                return -3
            else
                return -1
            endif
        endif

        let i = col('.') - 2
        let line = getline('.')
        while 1
            if (i == -1)
                break
            endif
            if (match(line[i], '[a-zA-Z0-9_\-]') == -1)
                break
            endif

            let i = i - 1
        endwhile
        let result = i + 1
        return result
    else
        python oc_compute_popup_list(True)
        call s:UpdatePopupMenuColorscheme()
        if (v:version > 702)
            return {'words' : g:omegacomplete_server_results, 'refresh' : 'always'}
        else
            return g:omegacomplete_server_results
        endif
    endif
endfunction

" substitute for VIM's taglist() function
function omegacomplete#taglist(expr)
    " send current directory to server in preparation for sending tags
    exe 'py current_directory = vim.eval("getcwd()")'
    exe 'py oc_eval("current_directory " + current_directory)'

    " send current tags we are using to the server
    exe 'py current_tags = vim.eval("&tags")'
    exe 'py oc_eval("taglist_tags " + current_tags)'

    " check if plugin has disabled itself because of connection problems
    " we can only do this only after 1 oc_eval() has occurred
    let is_oc_disabled=""
    exe 'py vim.command("let is_oc_disabled = " + oc_disable_check())'
    if (is_oc_disabled == "1")
        return []
    endif

    exe 'py oc_server_result = oc_eval("vim_taglist_function " + "' . a:expr . '")'

" check to make sure we get something
python << EOF
if len(oc_server_result) == 0:
    vim.command("return []")
EOF

    " try to show popup menu
    exe 'py vim.command("let taglist_results=" + oc_server_result)'

    return taglist_results
endfunction

function omegacomplete#UseFirstEntryOfPopup()
    if pumvisible()
        return "\<C-n>\<C-y>"
    else
        return "\<Tab>"
    endif
endfunction

function <SID>FlushServerCaches()
    exe 'py oc_eval("flush_caches 1")'
endfunction

" send config options to the C++ portion
function <SID>UpdateConfig()
    exe 'py oc_eval("config autocomplete_suffix ' .
        \ g:omegacomplete_autocomplete_suffix . '")'

    call <SID>ConfigureDisambiguateMode()

    if len(g:omegacomplete_log_file) > 0
      exe 'py oc_eval("set_log_file ' . g:omegacomplete_log_file . '")'
    endif
endfunction

function <SID>DoTests()
  python oc_eval("do_tests now")
endfunction

function <SID>ConfigureDisambiguateMode()
    " configure server
    exe 'py oc_eval("config server_side_disambiguate ' .
       \ g:omegacomplete_server_side_disambiguate . '")'

    " perform or remove mappings
    if (!g:omegacomplete_server_side_disambiguate)
        if g:omegacomplete_client_side_disambiguate_mappings
            let s:disambiguate_mode_keys =
                \ ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9',]

            for key in s:disambiguate_mode_keys
                exe printf('inoremap <silent> <A-%s> ' .
                    \ '<C-r>=<SID>PerformDisambiguate(%s)<CR>'
                    \ , key, key)
            endfor
        endif
    elseif (s:performed_client_disambiguate_mappings)
        for key in s:disambiguate_mode_keys
            exe 'iunmap ' . key
        endfor
        let s:disambiguate_mode_keys = []

        s:performed_client_disambiguate_mappings = 0
    endif
endfunction

" do initialization
call s:Init()
