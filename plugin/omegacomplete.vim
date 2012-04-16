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

function <SID>FileOpenNotification()
    let current_buffer_number = <SID>GetCurrentBufferNumber()
    let current_pathname = <SID>GetCurrentBufferPathname()
    "execute 'py oc_send_command("open_file", "' . current_buffer_number . '")'
endfunction

function <SID>CursorMovedINotification()
    let current_buffer_number = <SID>GetCurrentBufferNumber()
    let current_pathname = <SID>GetCurrentBufferPathname()

    execute 'py oc_send_command("current_buffer", "' . current_buffer_number . '")'
    execute 'py oc_send_command("current_pathname", "' . current_pathname . '")'
    execute 'py oc_send_command("buffer_contents", oc_get_current_buffer_contents())'
endfunction

augroup OmegaComplete
    autocmd!

    autocmd BufReadPost,BufWritePost,FileReadPost
    \ *
    \ call <SID>FileOpenNotification()

    autocmd CursorMovedI
    \ *
    \ call <SID>CursorMovedINotification()
augroup END

python << EOF
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

EOF
