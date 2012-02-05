"load guard
let g:omegacomplete_loaded = 1
if exists("g:omegacomplete_loaded")
  finish
endif
let g:omegacomplete_loaded = 1

function <SID>FileOpenNotification()
    let cmd = 'send_command("BufReadpost", "' . expand('%:p') . '")'
    execute 'python3 ' . cmd
endfunction
    
autocmd BufReadPost * call <SID>FileOpenNotification()

python3 << EOF
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
client_path = (omegacomplete_path + "/client.py")
exec(compile(open(client_path).read(), client_path, "exec"))
EOF
