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

" initialize this to be safe
let g:omegacomplete_server_results=[]

if !exists("g:omegacomplete_normal_hi_cmds")
    let g:omegacomplete_normal_hi_cmds=[
        \ "hi Pmenu guifg=#00ff00 guibg=#003700 gui=none",
        \ "hi PmenuSel guifg=#003700 guibg=#00ff00 gui=none",
        \ ]
endif
if !exists("g:omegacomplete_corrections_hi_cmds")
    let g:omegacomplete_corrections_hi_cmds=[
        \ "hi Pmenu guifg=#ffff00 guibg=#373700 gui=none",
        \ "hi PmenuSel guifg=#373700 guibg=#ffff00 gui=none",
        \ ]
endif

" whether teleprompter is active
let s:teleprompter = 0
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" init
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function s:Init()
    set completefunc=OmegaCompleteFunc

    set completeopt+=menu
    set completeopt+=menuone

    call s:MapForMappingDriven()
    nnoremap <silent> i i<C-r>=<SID>FeedPopup()<CR>
    nnoremap <silent> I I<C-r>=<SID>FeedPopup()<CR>
    nnoremap <silent> a a<C-r>=<SID>FeedPopup()<CR>
    nnoremap <silent> A A<C-r>=<SID>FeedPopup()<CR>
    nnoremap <silent> R R<C-r>=<SID>FeedPopup()<CR>

    command OmegaCompleteFlushServerCaches :call <SID>FlushServerCaches()

    " find and load the omegacomplete Python code
python << PYTHON
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
PYTHON
    return
endfunction

" utility functions that any script can use really
function <SID>EscapePathname(pathname)
    return substitute(a:pathname, '\\', '\\\\', 'g')
endfunction

function <SID>GetCurrentBufferNumber()
    return bufnr('%')
endfunction

function <SID>GetCurrentBufferPathname()
    return <SID>EscapePathname(expand('%:p'))
endfunction

" if you want imap's to trigger the popup menu
function s:MapForMappingDriven()
    call s:UnmapForMappingDriven()
    let s:keys_mapping_driven =
        \ [
        \ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        \ 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        \ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        \ 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        \ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        \ '_', '<Space>', '<C-h>', '<BS>', 
        \ ]

        "\ '-', '_', '~', '^', '.', ',', ':', '!', '#', '=', '%', '$', '@',
        "\ '<', '>', '/', '\'
 
    for key in s:keys_mapping_driven
        exe printf('inoremap <silent> %s %s<C-r>=<SID>FeedPopup()<CR>',
                 \ key, key)
    endfor
endfunction

" undo the effects of the previous function
function s:UnmapForMappingDriven()
    if !exists('s:keys_mapping_driven')
        return
    endif

    for key in s:keys_mapping_driven
        exe 'iunmap ' . key
    endfor
    let s:keys_mapping_driven = []
endfunction

function s:SendCurrentBuffer()
    python << PYTHON
oc_send_current_buffer()
PYTHON
endfunction

function <SID>FeedPopup()
    " disable when paste mode is active
    if &paste
        return ''
    endif
    
    " let server know what is the current buffer
    let buffer_number = <SID>GetCurrentBufferNumber()
    exe 'py oc_eval("current_buffer_id ' . buffer_number . '")'

    " check if plugin has disabled itself because of connection problems
    " we can only do this only after 1 oc_eval() has occurred
    let is_oc_disabled=''
    exe 'py vim.command("let is_oc_disabled = " + oc_disable_check())'
    if (is_oc_disabled == "1")
        return ''
    endif

    " send server the contents of the current line the cursor is at
    exe 'py oc_eval("current_line " + oc_get_current_line())'

    " tell server what the current cursor position is
    exe 'py oc_eval("cursor_position " + oc_get_cursor_pos())'

    " send server contents of the entire buffer in case reparse is needed
    if (s:just_did_insertenter != 1)
        call s:SendCurrentBuffer()
    endif
    let s:just_did_insertenter = 0

    " send current directory to server in preparation for sending tags
    exe 'py current_directory = vim.eval("getcwd()")'
    exe 'py oc_eval("current_directory " + current_directory)'

    " send tags we are using to the server
    exe 'py current_tags = vim.eval("&tags")'
    exe 'py oc_eval("current_tags " + current_tags)'

    " send current line up to the cursor
    let partial_line = strpart(getline('.'), 0, col('.') - 1)
    exe 'py oc_server_result = oc_eval("complete " + vim.eval("partial_line"))'

" check to make sure we get something
python << EOF
if len(oc_server_result) == 0:
    vim.command('let g:omegacomplete_server_results=[]')
else:
    vim.command('let g:omegacomplete_server_results=' + oc_server_result)
EOF

    if (len(g:omegacomplete_server_results) == 0)
        " try to show popup menu, but fail and reset completion status
        return "\<C-x>\<C-u>"
    else
        exe 'py is_corrections_only = oc_eval("is_corrections_only ?")'
        let is_corrections_only=0
python << EOF
if is_corrections_only == '0':
    vim.command('let is_corrections_only=0')
else:
    vim.command('let is_corrections_only=1')
EOF
        if (is_corrections_only)
            for cmd in g:omegacomplete_corrections_hi_cmds
                exe cmd
            endfor
        else
            for cmd in g:omegacomplete_normal_hi_cmds
                exe cmd
            endfor
        endif
        " show actual popup
        return "\<C-x>\<C-u>\<C-p>"
    endif
endfunction

" When we want to sync a buffer in normal mode. This mainly occurs when we
" are entering a buffer for the first time (i.e. it just opened), or we are
" switching windows and need it to be synced with the server in case you
" added or deleted lines
function <SID>NormalModeSyncBuffer()
    let buffer_number = <SID>GetCurrentBufferNumber()
    let buffer_name = bufname('%') 
    let absolute_path = expand('%:p')

    " help breaks omegacomplete by causing to forever remain
    if &ft == 'help'
        return
    endif
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
endfunction

function <SID>PruneBuffers()
    py oc_prune_buffers()
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
endfunction

function <SID>OnInsertLeave()
    if (s:teleprompter != 0)
        exe 'py oc_eval("hide_teleprompter 1")'
    endif
endfunction

function <SID>OnFocusLost()
    exe 'py oc_eval("prune 1")'
endfunction

function <SID>OnBufReadPost()
    call <SID>NormalModeSyncBuffer()
endfunction

augroup OmegaComplete
    autocmd!
    
    " whenever you delete a buffer, delete it from the server so that it
    " doesn't cause any outdated completions to be offered
    autocmd BufDelete * call <SID>OnBufDelete()
    
    " whenever we leave a current buffer, send it to the server to it can
    " decide if we have to update it. this keeps the state of the buffer
    " in sync so we can offer accurate completions when we switch to another
    " buffer
    autocmd BufLeave * call <SID>OnBufLeave()

    " just before you enter insert mode, send the contents of the buffer so
    " that the server has a chance to synchronize before we start offering
    " completions
    autocmd InsertEnter * call <SID>OnInsertEnter()

    " when you leave insert mode turn off any Teleprompter if possible
    autocmd InsertLeave * call <SID>OnInsertLeave()

    " when you switch from the VIM window to some other process, tell the
    " server to prune unused words from its completion set
    autocmd FocusLost * call <SID>OnFocusLost()

    " when we open a file, send it contents to the server since we usually
    " have some time before we need to start editing the file (usually you
    " have to mentally parse where you are and/or go to the correct location
    " before entering insert mode)
    autocmd BufReadPost * call <SID>OnBufReadPost()
augroup END

" this needs to have global scope and it is what <C-X><C-U> depends on.
" don't think  it can use script / scope specific functions
function OmegaCompleteFunc(findstart, base)
    if a:findstart
        let index = col('.') - 2
        let line = getline('.')
        while 1
            if (index == -1)
                break
            endif
            if ( match(line[index], '[a-zA-Z0-9_]') == -1 )
                break
            endif

            let index = index - 1
        endwhile
        let result = index + 1
        return result
    else
        if (v:version > 702)
            return {'words' : g:omegacomplete_server_results}
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
python << PYTHON
if len(oc_server_result) == 0:
    vim.command("return []")
PYTHON

    " try to show popup menu
    exe 'py vim.command("let taglist_results=" + oc_server_result)'

    return taglist_results
endfunction

function omegacomplete#UseFirstEntryOfPopup()
    if (s:teleprompter != 0)
        exe 'py oc_eval("hide_teleprompter 1")'
    endif

    if pumvisible()
        return "\<C-n>\<C-y>"
    else
        let line = getline('.')
        let index = col('.') - 2
        if (index <= 0)
            return "\<Tab>"
        elseif (line[index] == '.')
            return <SID>FeedPopup()
        elseif (index >= 2 && line[index - 1] == '-' && line[index] == '>')
            return <SID>FeedPopup()
        elseif (index >= 2 && line[index - 1] == ':' && line[index] == ':')
            return <SID>FeedPopup()
        else
            return "\<Tab>"
        endif
    endif
endfunction

function <SID>FlushServerCaches()
    exe 'py oc_eval("flush_caches 1")'
endfunction

" do initialization
call s:Init()
