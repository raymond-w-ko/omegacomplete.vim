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
      \ '__Gundo_Preview__',
      \ ]
endif

if !exists("g:omegacomplete_quick_select")
    let g:omegacomplete_quick_select=1
endif
if !exists("g:omegacomplete_quick_select_keys")
    let g:omegacomplete_quick_select_keys="1234567890"
endif

let g:omegacomplete_server_results = []
let g:omegacomplete_filesize_limit = 1024 * 1024
let g:omegacomplete_completion_delay = 256
let g:omegacomplete_auto_close_doc = 1

let s:char_inserted = v:false
let s:ignored_buffer_names_set = {}
let s:completion_begin_col = -1

function omegacomplete#completefunc(findstart, base)
  " on first invocation a:findstart is 1 and base is empty
  if a:findstart
    if s:completion_begin_col < 0
        return -2
    else
      return s:completion_begin_col
    endif
  else
    call s:UpdatePopupMenuColorscheme()
    return {'words' : g:omegacomplete_server_results, 'refresh' : 'always'}
  endif
endfunction

function s:consistent()
  return v:true
endfunction

function omegacomplete#trigger()
  let is_empty = v:false
  if !s:consistent()
    let is_empty = v:true
  else
    python oc_update_current_buffer_info()
    python oc_get_word_begin_index()
    if s:completion_begin_col >= 0
      python oc_compute_popup_list()
    endif

    if len(g:omegacomplete_server_results) == 0
      let is_empty = v:true
    endif
  endif
  if is_empty | return | endif

  setlocal completefunc=omegacomplete#completefunc
  " due to the various plugins overriding these settings, force this
  setlocal completeopt-=longest
  setlocal completeopt+=menuone
  setlocal completeopt-=menu
  setlocal completeopt+=noselect
  call feedkeys("\<Plug>OmegacompleteTrigger")
endfunction

function omegacomplete#do_complete()
  call omegacomplete#trigger()
endfunction

function s:reset()
  let g:omegacomplete_server_results = []
endfunction

function s:complete(...)
  call s:reset()
  call omegacomplete#do_complete()
endfunction

function s:skip()
  let buftype = &buftype
  let name = bufname("")
  let fsize = getfsize(name)
  let skip = 0
      \ || (buftype ==? "quickfix")
      \ || (fsize == -2) || fsize > g:omegacomplete_filesize_limit
      \ || has_key(s:ignored_buffer_names_set, name)
  return skip || !s:char_inserted
endfunction

function s:on_text_change()
  if s:skip() | return | endif
  let s:char_inserted = v:false

  if exists("s:timer")
    call timer_stop(s:timer)
  endif

  let s:timer = timer_start(g:omegacomplete_completion_delay, function("s:complete"))
endfunction

function s:on_insert_char_pre()
  let s:char_inserted = v:true
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" init
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function s:setup_commands()
  command OmegacompleteFlushServerCaches :call <SID>FlushServerCaches()
  command OmegacompleteUpdateConfig :call <SID>UpdateConfig()
  command OmegacompleteDoTests :call <SID>DoTests()
endfunction

function s:setup_keybinds()
  noremap  <silent> <Plug>OmegacompleteTrigger <nop>
  inoremap <silent> <Plug>OmegacompleteTrigger <c-x><c-u><c-p>

  if g:omegacomplete_quick_select
    let n = strlen(g:omegacomplete_quick_select_keys) - 1
    for i in range(0, n, 1)
      let key = g:omegacomplete_quick_select_keys[i]
      let cmd = printf("inoremap <silent> %s \<C-r>=omegacomplete#quick_select('%s', %d)\<CR>", key, key, i+1)
      exe cmd
    endfor
  endif
endfunction

function s:setup_events()
  augroup omegacomplete
    autocmd!

    autocmd TextChangedI * call s:on_text_change()
    autocmd InsertCharPre * call s:on_insert_char_pre()
    if get(g:, "omegacomplete_auto_close_doc", 1)
      autocmd CompleteDone * if pumvisible() == 0 | pclose | endif
    endif

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
    " autocmd CursorHoldI * call <SID>SyncCurrentBuffer()
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
    " autocmd CursorMovedI * call <SID>OnCursorMovedI()
  augroup END
endfunction

function omegacomplete#enable()
  " load Python <---> C++ plugin
  if !s:load_core_plugin()
    echom "failed to load omegacomplete core plugin"
    return
  endif

  " convert list to set
  for buffer_name in g:omegacomplete_ignored_buffer_names
    let s:ignored_buffer_names_set[buffer_name] = 1
  endfor

  call s:setup_commands()
  call s:setup_keybinds()
  call s:setup_events()

  call <SID>UpdateConfig()
  call s:UpdatePopupMenuColorscheme()
endfunction

function omegacomplete#disable()
  autocmd! omegacomplete
endfunction

function s:load_core_plugin()
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
    call <SID>SyncCurrentBuffer()
endfunction

function <SID>OnInsertEnter()
  let s:just_did_insertenter = 1
  call <SID>SyncCurrentBuffer()
  call <SID>PruneBuffers()
endfunction

function <SID>OnIdle()
  call <SID>PruneBuffers()
  call <SID>Prune()
endfunction

function <SID>OnBufReadPost()
    call <SID>SyncCurrentBuffer()
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

call omegacomplete#enable()

let &cpo = s:save_cpo
unlet s:save_cpo
