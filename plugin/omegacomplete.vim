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
    let cmd = 'send_command("file_open", "' . current_filename . '")'

    execute 'python ' . cmd
endfunction

function <SID>CursorMovedINotification()
    let current_filename = <SID>EscapePathname(expand('%:p'))

    let cmd = 'send_command("current_file", "' . current_filename . '")'
    execute 'python ' . cmd

    let cmd = 'send_command("buffer_contents", get_current_buffer_contents())'
    execute 'python ' . cmd
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

EOF
