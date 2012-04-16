"load guard
if exists('g:omegacomplete_loaded')
  finish
endif
let g:omegacomplete_loaded = 1

function <SID>EscapePathname(pathname)
    return substitute(a:pathname, '\\', '\\\\', 'g')
endfunction

function <SID>FileOpenNotification()
    let current_filename = <SID>EscapePathname(expand('%:p'))
    "execute 'py oc_send_command("open_file", "' . current_filename . '")'
endfunction

function <SID>CursorMovedINotification()
    let current_filename = <SID>EscapePathname(expand('%:p'))

    execute 'py oc_send_command("current_buffer", "' . current_filename . '")'
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
