let s:save_cpo = &cpo
set cpo&vim

" script load guard
if exists('g:loaded_omegacomplete')
    finish
endif
let g:loaded_omegacomplete = 1

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" config options
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" this variable acts a guard to prevent needlessly sending the same
" buffer contents twice in a row
let s:just_did_insertenter = 0

" path of log file to write debug output
" right now the only thing that uses this the stopwatch
if !exists('g:omegacomplete_log_file')
  let g:omegacomplete_log_file = ''
endif

" current hilight mode (either normal or corrections)
let s:current_hi_mode = 'unitialized'

let g:omegacomplete_is_corrections_only=0
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

if !exists("g:omegacomplete_ignored_buffer_names")
    let g:omegacomplete_ignored_buffer_names = [
        \ '__Scratch__',
        \ '__Gundo__',
        \ 'GoToFile',
        \ 'ControlP',
        \ 'ControlP',
        \ '__Gundo_Preview__',
        \ ]
endif
let s:ignored_buffer_names_set = {}

if !exists("g:omegacomplete_quick_select")
    let g:omegacomplete_quick_select=1
endif
if !exists("g:omegacomplete_quick_select_keys")
    let g:omegacomplete_quick_select_keys="1234567890"
endif

" variables to keep track of cursor so infinite completion loop by CursorMovedI is avoided
let s:last_completed_row = -1
let s:last_completed_col = -1

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" init
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function s:Init()
    if !s:loadCorePlugin()
        echom "failed to load omegacomplete core plugin"
        return
    endif

    " initialize this to be safe, where server results are stored
    let g:omegacomplete_server_results=[]
    let g:omegacomplete_word_begin_index=-1

    " convert list to set
    for buffer_name in g:omegacomplete_ignored_buffer_names
        let s:ignored_buffer_names_set[buffer_name] = 1
    endfor

    set completefunc=omegacomplete#Complete

    " we pretty much have to force this otherwise this plugin is not helpful.
    " copypasta from YouCompleteMe
    "
    " Some plugins (I'm looking at you, vim-notes) change completeopt by for
    " instance adding 'longest'. This breaks YCM. So we force our settings.
    " There's no two ways about this: if you want to use YCM then you have to
    " have these completeopt settings, otherwise YCM won't work at all.
    "
    " We need menuone in completeopt, otherwise when there's only one candidate
    " for completion, the menu doesn't show up.
    set completeopt-=menu
    set completeopt+=menuone
    " This is unnecessary with our features. People use this option to insert
    " the common prefix of all the matches and then add more differentiating chars
    " so that they can select a more specific match. With our features, they
    " don't need to insert the prefix; they just type the differentiating chars.
    " Also, having this option set breaks the plugin.
    set completeopt-=longest
    if v:version > 704 || (v:version == 704 && has('patch775'))
        set completeopt+=noselect
    endif

    command OmegacompleteFlushServerCaches :call <SID>FlushServerCaches()
    command OmegacompleteUpdateConfig :call <SID>UpdateConfig()
    command OmegacompleteDoTests :call <SID>DoTests()

    call <SID>UpdateConfig()
    call s:UpdatePopupMenuColorscheme()

    if g:omegacomplete_quick_select
        let n = strlen(g:omegacomplete_quick_select_keys) - 1
        for i in range(0, n, 1)
            let key = g:omegacomplete_quick_select_keys[i]
            let cmd = printf("inoremap %s \<C-r>=omegacomplete#quick_select('%s', %d)\<CR>", key, key, i+1)
            exe cmd
        endfor
    endif

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
        autocmd CursorHoldI * call <SID>SyncCurrentBuffer()
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

function <SID>OnCursorMovedI()
    " disable when paste mode is active, to avoid massive processing lag
    if &paste
        return
    endif

    let c = col('.')
    let r = line('.')
    if s:last_completed_row == r && s:last_completed_col == c
        return
    endif
    let s:last_completed_row = r
    let s:last_completed_col = c

    let line = getline('.')
    if len(line) == 0
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

" When we want to sync a buffer in normal mode. This mainly occurs when we
" are entering a buffer for the first time (i.e. it just opened), or we are
" switching windows and need it to be synced with the server in case you
" added or deleted lines
function <SID>SyncCurrentBuffer()
    let buffer_number = bufnr('%')
    let buffer_name = bufname('%')
    let absolute_path = escape(expand('%:p'), '\')

    " don't process these special buffers from other plugins
    if has_key(s:ignored_buffer_names_set, buffer_name)
        return
    endif

    exe 'py oc_eval("current_buffer_id ' . buffer_number . '")'
    exe 'py oc_eval("current_buffer_absolute_path ' . absolute_path . '")'
    python oc_send_current_buffer()
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
    let s:last_completed_row = -1
    let s:last_completed_col = -1
    call <SID>SyncCurrentBuffer()
endfunction

function <SID>OnInsertEnter()
    let s:just_did_insertenter = 1
    call <SID>SyncCurrentBuffer()
    call <SID>PruneBuffers()
    " this break . command, e.g. * then cw<ESC>n.n.n.
    " causes strange jumping, maybe I have to hook into vim-repeat?
    " call <SID>OnCursorMovedI()
endfunction

function <SID>OnIdle()
    call <SID>PruneBuffers()
    call <SID>Prune()
endfunction

function <SID>OnBufReadPost()
    call <SID>SyncCurrentBuffer()
endfunction

" this needs to have global scope and it is what <C-X><C-U> depends on.
" don't think  it can use script / scope specific functions
function omegacomplete#Complete(findstart, base)
    " on first invocation a:findstart is 1 and base is empty
    if a:findstart
        python oc_update_current_buffer_info()
        python oc_get_word_begin_index()

        " this should only be triggered if there is a valid initial match
        if g:omegacomplete_word_begin_index < 0
            if (v:version > 702)
                return -3
            else
                return -1
            endif
        else
            return g:omegacomplete_word_begin_index
        endif
    else
        python oc_compute_popup_list()
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

function omegacomplete#quick_select(key, index)
    if !pumvisible()
        return a:key
    else
        let keys = ""
        for i in range(a:index)
            let keys .= "\<C-n>"
        endfor
        let keys .= "\<C-y>"
        return keys
    endif
endfunction

" send config options to the C++ portion
function <SID>UpdateConfig()
    if len(g:omegacomplete_log_file) > 0
      exe 'py oc_eval("set_log_file ' . g:omegacomplete_log_file . '")'
    endif
    exe 'py oc_eval("set_quick_select_keys ' . g:omegacomplete_quick_select_keys . '")'
endfunction

function <SID>DoTests()
  python oc_eval("do_tests now")
endfunction

" do initialization
call s:Init()

let &cpo = s:save_cpo
unlet s:save_cpo
