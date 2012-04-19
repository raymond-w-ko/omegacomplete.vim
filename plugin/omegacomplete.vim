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
		\ '_',
		\ '<Space>', '<C-h>', '<BS>', ]

		"\ '-', '_', '~', '^', '.', ',', ':', '!', '#', '=', '%', '$', '@', '<', '>', '/', '\',
	for key in s:keysMappingDriven
		execute printf('inoremap <silent> %s %s<C-r>=<SID>feedPopup()<CR>',
			\          key, key)
	endfor
endfunction

function s:unmapForMappingDriven()
	if !exists('s:keysMappingDriven')
		return
	endif
	for key in s:keysMappingDriven
		execute 'iunmap ' . key
	endfor
	let s:keysMappingDriven = []
endfunction

function <SID>feedPopup()
	if &paste
		return ''
	endif
	
    let current_buffer_number = <SID>GetCurrentBufferNumber()
    let current_pathname = <SID>GetCurrentBufferPathname()

	" synchronize current buffer
    execute 'py oc_send_command("current_buffer ' . current_buffer_number . '")'
    execute 'py oc_send_command("current_pathname ' . current_pathname . '")'
    execute 'py oc_send_command("current_line " + oc_get_current_line())'
    execute 'py oc_send_command("buffer_contents_insert_mode " + oc_get_current_buffer_contents())'
	
	let is_oc_disabled=""
	execute 'py vim.command("let is_oc_disabled=" + oc_disable_check())'
	if (is_oc_disabled == "1")
		return ''
	endif
	
	" request completion
	let cursor_row = line('.')
	let cursor_col = col('.') - 1
	execute 'py oc_send_command("cursor_position " + str(' . cursor_row . ') + " " + str(' . cursor_col . '))'

    let partial_line = strpart(getline('.'), 0, col('.') - 1)
	execute 'py oc_server_result = oc_send_command("complete " + vim.eval("partial_line"))'

python << EOF
if len(oc_server_result) == 0:
	vim.command("return ''")
EOF

	execute 'py vim.command("let server_result=" + oc_server_result)'
	if (len(server_result) == 0)
		return ''
	endif
	let g:omegacomplete_server_results = server_result
	call feedkeys("\<C-X>\<C-U>\<C-P>", 'n')
	
	return ''
endfunction

function <SID>CursorMovedNotification()
    let current_buffer_number = <SID>GetCurrentBufferNumber()
    let current_pathname = <SID>GetCurrentBufferPathname()

	" synchronize current buffer
    execute 'py oc_send_command("current_buffer ' . current_buffer_number . '")'
    execute 'py oc_send_command("current_pathname ' . current_pathname . '")'
    execute 'py oc_send_command("buffer_contents " + oc_get_current_buffer_contents())'
endfunction

augroup OmegaComplete
    autocmd!

    "autocmd CursorMovedI
    "\ *
    "\ call <SID>CursorMovedINotification()

    autocmd CursorMoved,InsertLeave
    \ *
    \ call <SID>CursorMovedNotification()
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
        return { 'words' : g:omegacomplete_server_results, 'refresh' : 'always' }
    endif
endfunction

" init
set completefunc=OmegaCompleteFunc
call s:mapForMappingDriven()

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
