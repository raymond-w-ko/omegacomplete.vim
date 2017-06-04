" vim: et ts=2 sts=2 sw=2
let s:save_cpo = &cpo
set cpo&vim

if has("python")
  let s:_py = ":python"
  let s:_pyfile = ":pyfile"
elseif has("python3")
  let s:_py = ":python3"
  let s:_pyfile = ":py3file"
endif

let s:python_dir = fnamemodify(expand('<sfile>'), ':p:h:h') . "/python"
let s:current_hi_mode = v:null
let s:is_corrections_only=0
let s:char_inserted = v:false
let s:buffer_name_blacklist = {}
let s:completion_begin_col = -1
let s:completions = []
let s:status = {"pos": [], "nr": -1, "input": "", "ft": ""}

function omegacomplete#completefunc(findstart, base)
  " on first invocation a:findstart is 1 and base is empty
  if a:findstart
    if s:completion_begin_col < 0
        return -2
    else
      return s:completion_begin_col
    endif
  else
    call s:update_popup_menu_color_scheme()
    return {'words' : s:completions, 'refresh' : 'always'}
  endif
endfunction

function s:consistent()
  return s:status.nr == bufnr('') && s:status.pos == getcurpos() && s:status.ft == &ft
endfunction

function omegacomplete#trigger()
  let is_empty = v:false
  if !s:consistent()
    let is_empty = v:true
  else
    exe s:_py "oc_update_current_buffer_info()"
    exe s:_py "oc_get_word_begin_index()"
    if s:completion_begin_col >= 0
      exe s:_py "oc_compute_popup_list()"
    endif

    if len(s:completions) == 0
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
  let s:completions = []
  " TODO stop jobs here
endfunction

function s:complete(...)
  call s:reset()
  if !s:consistent() | return | endif

  call omegacomplete#do_complete()
endfunction

function s:skip()
  let buftype = &buftype
  let name = bufname("")
  let fsize = getfsize(name)
  let skip = 0
      \ || (buftype ==? "quickfix")
      \ || (fsize == -2) || fsize > g:omegacomplete_filesize_limit
      \ || has_key(s:buffer_name_blacklist, name)
  return skip || !s:char_inserted
endfunction

function s:on_text_change()
  if s:skip() | return | endif
  let s:char_inserted = v:false

  if exists("s:timer")
    call timer_stop(s:timer)
  endif

  let x = col(".") - 2
  let inputted = x >= 0 ? getline(".")[:x] : ""

  let s:status = {'input': inputted, 'pos': getcurpos(), 'nr': bufnr(''), 'ft': &ft}
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
  command OmegacompleteUpdateConfig :call <SID>update_config()
  command OmegacompleteDoTests :call <SID>do_tests()
endfunction

function s:setup_keybinds()
  noremap  <silent> <Plug>OmegacompleteTrigger <nop>
  inoremap <silent> <Plug>OmegacompleteTrigger <c-x><c-u><c-p>

  if g:omegacomplete_enable_quick_select
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
    autocmd BufDelete * call <SID>on_buf_delete()

    " whenever we leave a current buffer, send it to the server to it can
    " decide if we have to update it. this keeps the state of the buffer
    " in sync so we can offer accurate completions when we switch to
    " another buffer
    autocmd BufLeave * call <SID>on_buf_leave()

    " just before you enter insert mode, send the contents of the buffer
    " so that the server has a chance to synchronize before we start
    " offering completions
    autocmd InsertEnter * call <SID>on_insert_enter()

    " if we are idle, then we take this change to prune unused global
    " words set and trie. there are a variety of things which we consider
    " 'idle'
    autocmd CursorHold * call <SID>on_idle()
    autocmd CursorHoldI * call <SID>on_idle()
    autocmd CursorHoldI * call <SID>sync_current_buffer()
    autocmd FocusLost * call <SID>on_idle()
    autocmd BufWritePre * call <SID>on_idle()

    " when we open a file, send it contents to the server since we usually
    " have some time before we need to start editing the file (usually you
    " have to mentally parse where you are and/or go to the correct
    " location before entering insert mode)
    autocmd BufReadPost * call <SID>on_buf_read_post()
  augroup END
endfunction

function omegacomplete#enable()
  " load Python <---> C++ plugin
  if !s:load_core_plugin()
    echom "failed to load omegacomplete C++ core plugin"
    return
  endif

  " convert list to set
  for buffer_name in g:omegacomplete_buffer_name_blacklist
    let s:buffer_name_blacklist[buffer_name] = 1
  endfor

  call s:setup_commands()
  call s:setup_keybinds()
  call s:setup_events()

  call <SID>update_config()
  call s:update_popup_menu_color_scheme()
endfunction

function omegacomplete#disable()
  autocmd! omegacomplete
endfunction

function s:load_core_plugin()
  exe s:_pyfile s:python_dir."/omegacomplete/helper.py"
  return s:loaded_core_module
endfunction

" When we don't want a buffer loaded in memory in VIM, we can 'delete' the
" buffer. This must be reflected on the server, otherwise we get completions
" that we no longer want.
function <SID>on_buf_delete()
  exe s:_py "oc_free_current_buffer()"
  call <SID>prune()
endfunction

" Not that expensive, just checks for dead buffers and kills them if they are
" not in the set of alive buffers
function <SID>prune_buffers()
    exe s:_py "oc_prune_buffers()"
endfunction

" Fairly expensive as it has to rebuild it's list of words and shuffle them.
function <SID>prune()
    exe s:_py "oc_prune()"
endfunction

" When we are leaving buffer (either by opening another buffer, switching
" tabs, or moving to another split), then we have to inform send the contents
" of the buffer to the server since we could have changed the contents of the
" buffer while in normal though commands like 'dd'
function <SID>on_buf_leave()
    call <SID>sync_current_buffer()
endfunction

function <SID>on_insert_enter()
  call <SID>sync_current_buffer()
  call <SID>prune_buffers()
endfunction

function <SID>on_idle()
  call <SID>prune_buffers()
  call <SID>prune()
endfunction

function <SID>on_buf_read_post()
    call <SID>sync_current_buffer()
endfunction

""""""""""""""""""""""""""""""""""""""""

function s:update_popup_menu_color_scheme()
  if s:is_corrections_only && s:current_hi_mode != 'corrections'
    for cmd in g:omegacomplete_corrections_hi_cmds
      exe cmd
    endfor
    let s:current_hi_mode = 'corrections'
  elseif !s:is_corrections_only && s:current_hi_mode != 'normal'
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
function <SID>sync_current_buffer()
  " don't process these special buffers from other plugins
  let buffer_name = bufname('%')
  if has_key(s:buffer_name_blacklist, buffer_name)
    return
  endif

  exe s:_py "oc_send_current_buffer()"
endfunction

function! <SID>update_config()
  exe s:_py "oc_update_config()"
endfunction

function! <SID>do_tests()
  exe s:_py "oc_do_tests()"
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" public functions
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function omegacomplete#use_first_entry_of_popup()
  if pumvisible()
    return "\<C-n>\<C-y>"
  else
    return "\<Tab>"
  endif
endfunction

function <SID>FlushServerCaches()
  exe s:_py "oc_flush_caches()"
endfunction

function! omegacomplete#quick_select(key, index)
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

" a substitute for VIM's taglist() function
function omegacomplete#taglist(expr)
  let l:taglist_results = []
  exe s:_py "oc_taglist()"
  return l:taglist_results
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

let &cpo = s:save_cpo
unlet s:save_cpo
