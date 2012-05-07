"load guard
if exists('g:omegacomplete_loaded')
  finish
endif
let g:omegacomplete_loaded = 1

function <SID>EscapePathname(pathname)
    return substitute(a:pathname, '\\', '\\\\', 'g')
endfunction

function <SID>GetCurrentBufferNumber()
    return bufnr('%')
endfunction

function <SID>GetCurrentBufferPathname()
    return <SID>EscapePathname(expand('%:p'))
endfunction

" interface to the server

function s:mapForMappingDriven()
    call s:unmapForMappingDriven()
    let s:keysMappingDriven = [
        \ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        \ 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        \ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        \ 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        \ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        \ '_', '<Space>', '<C-h>', '<BS>', ]

        "\ '-', '_', '~', '^', '.', ',', ':', '!', '#', '=', '%', '$', '@',
        "\ '<', '>', '/', '\'
 
    for key in s:keysMappingDriven
        exe printf('inoremap <silent> %s %s<C-r>=<SID>FeedPopup()<CR>',
                     \ key, key)
    endfor
endfunction

function s:unmapForMappingDriven()
    if !exists('s:keysMappingDriven')
        return
    endif
    for key in s:keysMappingDriven
        exe 'iunmap ' . key
    endfor
    let s:keysMappingDriven = []
endfunction

function <SID>FeedPopup()
    " disable when paste mode is active
    if &paste
        return ''
    endif
    
    " let server know what is the current buffer
    let current_buffer_number = <SID>GetCurrentBufferNumber()
    exe 'py oc_send_command("current_buffer ' . current_buffer_number . '")'

    " check if plugin has disabled itself because of connection problems
    " we can only do this only after 1 oc_send_command() has occurred
    let is_oc_disabled=""
    exe 'py vim.command("let is_oc_disabled = " + oc_disable_check())'
    if (is_oc_disabled == "1")
        return ''
    endif

    " send server the contents of the current line the cursor is at
    exe 'py oc_send_command("current_line " + oc_get_current_line())'

    " tell server what the current cursor position is
    exe 'py oc_send_command("cursor_position " + oc_get_cursor_pos())'

    " send server contents of the entire buffer in case reparse is needed
    exe 'py oc_send_command("buffer_contents_insert_mode " + oc_get_current_buffer_contents())'

    " send tags we are using to the server
    exe 'py current_tags = vim.eval("&tags")'
    exe 'py oc_send_command("current_tags " + current_tags)'

    " send current line up to the cursor
    let partial_line = strpart(getline('.'), 0, col('.') - 1)
    exe 'py oc_server_result = oc_send_command("complete " + vim.eval("partial_line"))'

" check to make sure we get something
python << PYTHON
if len(oc_server_result) == 0:
    vim.command("return ''")
PYTHON

    " try to show popup menu
    exe 'py vim.command("let g:omegacomplete_server_results=" + oc_server_result)'
    if (len(g:omegacomplete_server_results) == 0)
        " do a failed popup anyway to prevent double Enter bug
        call feedkeys("\<C-X>\<C-U>", 't')
        return ''
    else
        " show actual popup
        call feedkeys("\<C-X>\<C-U>\<C-P>", 't')
        return ''
    endif
endfunction

function <SID>NormalModeSyncBuffer()
    let current_buffer_number = <SID>GetCurrentBufferNumber()

    exe 'py oc_send_command("current_buffer ' . current_buffer_number . '")'
    exe 'py oc_send_command("buffer_contents " + oc_get_current_buffer_contents())'
endfunction

" When we don't want a buffer loaded in memory in VIM, we can 'delete' the
" buffer. This must be reflected on the server, otherwise we get completions
" that we no longer want.
function <SID>OnBufDelete()
    let current_buffer_number = expand('<abuf>')
    exe 'py oc_send_command("free_buffer ' . current_buffer_number . '")'
endfunction

" When we are leaving buffer (either by opening another buffer, switching
" tabs, or moving to another split), then we have to inform send the contents
" of the buffer to the server since we could have changed the contents of the
" buffer while in normal though commands like 'dd'
function <SID>OnBufLeave()
    call <SID>NormalModeSyncBuffer()
endfunction

function <SID>OnInsertEnter()
    call <SID>NormalModeSyncBuffer()
endfunction

augroup OmegaComplete
    autocmd!
    
    " whenever you delete a buffer, remove it so that it doesn't introduce
    " any false positive completions
    autocmd BufDelete
    \ *
    \ call <SID>OnBufDelete()
    
    " whenever we leave a current buffer, send it to the server to it can
    " decide if we have to update it. I'm going to to try going this route
    " so we can defer buffer processing until we leave or enter into insert mode
    autocmd BufLeave
    \ *
    \ call <SID>OnBufLeave()

    " just before you enter insert mode send the contents of the buffer so
    " that the server has a chance to synchronize before popping up the
    " insertion completion menu
    autocmd InsertEnter
    \ *
    \ call <SID>OnInsertEnter()
augroup END

function <SID>IsPartOfWord(character)
    if (match(a:character, '[a-zA-Z0-9_]') == 0)
        return 1
    else
        return 0
    endif
endfunction

function OmegaCompleteFunc(findstart, base)
    if a:findstart
        let index = col('.') - 2
        let line = getline('.')
        while 1
            if (index == -1)
                break
            endif
            if ( <SID>IsPartOfWord( line[index] ) == 0 )
                break
            endif

            let index = index - 1
        endwhile
        return index + 1
    else
        return { 'words' : g:omegacomplete_server_results }
    endif
endfunction

function omegacomplete#UseFirstWordOfPmenu()
    if pumvisible() == 0
        return ''
    endif
    return "\<C-n>\<C-y>"
endfunction

function omegacomplete#taglist(expr)
    " send current tags we are using to the server
    exe 'py current_tags = vim.eval("&tags")'
    exe 'py oc_send_command("taglist_tags " + current_tags)'

    exe 'py oc_server_result = oc_send_command("vim_taglist_function " + "' . a:expr . '")'

" check to make sure we get something
python << PYTHON
if len(oc_server_result) == 0:
    vim.command("return []")
PYTHON

    " try to show popup menu
    exe 'py vim.command("let taglist_results=" + oc_server_result)'

    return taglist_results
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" init
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set completefunc=OmegaCompleteFunc
call s:mapForMappingDriven()
nnoremap <silent> i i<C-r>=<SID>FeedPopup()<CR>
nnoremap <silent> I I<C-r>=<SID>FeedPopup()<CR>
nnoremap <silent> a a<C-r>=<SID>FeedPopup()<CR>
nnoremap <silent> A A<C-r>=<SID>FeedPopup()<CR>
nnoremap <silent> R R<C-r>=<SID>FeedPopup()<CR>

python << PYTHON
import vim
import os

# find omegacomplete directory
path_list = vim.eval("&runtimepath").split(",")
omegacomplete_path = ""
for path in path_list:
    candidate_path = path + "/python/omegacomplete"
    if (os.path.exists(candidate_path)):
        omegacomplete_path = candidate_path

sys.path.append(omegacomplete_path)

# load omegacomplete python functions
client_path = omegacomplete_path + "/client.py"
exec(compile(open(client_path).read(), client_path, "exec"))

oc_init_connection()

PYTHON
