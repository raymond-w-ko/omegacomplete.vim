" vim: et ts=2 sts=2 sw=2

let s:save_cpo = &cpo
set cpo&vim

function! s:restore_cpo()
  let &cpo = s:save_cpo
  unlet s:save_cpo
endfunction

function! s:has_features()
	return
      \ ((has("python") || has("python3")) && has("job") && has("timers") && has("lambda"))
      \ || has('nvim')
endfunction

if exists("g:loaded_omegacomplete_plugin")
  call s:restore_cpo()
  finish
elseif !s:has_features()
  echohl WarningMsg
  echomsg "Omegacomplete requires vim compiled with python or python3 and has features `job`, `timers`, and `lambda`"
  echohl None
  call s:restore_cpo()
  finish
endif
let g:loaded_omegacomplete_plugin = 1

let s:default_normal_highlight = [
    \ "hi Pmenu guifg=#00ff00 guibg=#003300 gui=none " .
    \          "ctermbg=22 ctermfg=46 term=none cterm=none",
    \ "hi PmenuSel guifg=#003300 guibg=#00ff00 gui=none " .
    \             "ctermbg=46 ctermfg=22 term=none cterm=none",
    \ ]
let s:default_corrections_highlight = [
    \ "hi Pmenu guifg=#ffff00 guibg=#333300 gui=none " .
    \          "ctermbg=058 ctermfg=226 term=none cterm=none",
    \ "hi PmenuSel guifg=#333300 guibg=#ffff00 gui=none " .
    \             "ctermbg=226 ctermfg=058 term=none cterm=none",
    \ ]

let s:default_buffer_name_blacklist = [
    \ '__Scratch__',
    \ '__Gundo__',
    \ 'GoToFile',
    \ 'ControlP',
    \ '__Gundo_Preview__',
    \ '[Command Line]',
    \ ]

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" config options
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" path of log file to write debug output
" right now the only thing that uses this is the stopwatch
let g:omegacomplete_log_file = get(g:, "omegacomplete_log_file", "")

" popup menu color schemes
let g:omegacomplete_normal_hi_cmds =
    \ get(g:, "omegacomplete_normal_hi_cmds", s:default_normal_highlight)
let g:omegacomplete_corrections_hi_cmds =
    \ get(g:, "omegacomplete_normal_hi_cmds", s:default_corrections_highlight)

" buffer name blacklist (used to pretend these buffers don't exist)
let g:omegacomplete_buffer_name_blacklist =
    \ get(g:, "omegacomplete_buffer_name_blacklist", s:default_buffer_name_blacklist)

" quick select completions
let g:omegacomplete_enable_quick_select =
    \ get(g:, "omegacomplete_enable_quick_select", 1)
let g:omegacomplete_quick_select_keys =
    \ get(g:, "omegacomplete_quick_select_keys", "1234567890")

" file size limit in bytes
let g:omegacomplete_filesize_limit =
    \ get(g:, "omegacomplete_filesize_limit", 1024 * 1024)

" time delay before popping up completion menu (in milliseconds)
let g:omegacomplete_completion_delay =
    \ get(g:, "omegacomplete_completion_delay", 256)

" auto-close preview window when popup closes (not used currently)
let g:omegacomplete_auto_close_doc = get(g:, "omegacomplete_auto_close_doc", 1)

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

command! OmegacompleteEnable call omegacomplete#enable()
command! OmegacompleteDisable call omegacomplete#disable()

call omegacomplete#enable()

call s:restore_cpo()
