# these are probably global to the VIM Python interpreter, so prefix with "oc_"
# to prevent conflicts with any other Python plugins

oc_core_plugin_load_exception = False
try:
    from omegacomplete.core import eval as oc_core_eval
except Exception as e:
    oc_core_plugin_load_exception = True

def oc_is_disabled():
    global oc_core_plugin_load_exception
    if oc_core_plugin_load_exception:
        return "0"
    else:
        return "1"

def oc_eval(cmd):
    return oc_core_eval(cmd)

def oc_update_config():
    log_file_path = vim.eval("g:omegacomplete_log_file")
    if len(log_file_path) > 0:
        oc_eval("set_log_file " + log_file_path)
    quick_select_keys = vim.eval("g:omegacomplete_quick_select_keys")
    oc_eval("set_quick_select_keys " + quick_select_keys)

def oc_send_current_buffer():
    # send iskeyword so we can be smarter on what is considered a word
    oc_core_eval("current_iskeyword " + vim.eval("&iskeyword"))
    # this approach takes around 0.8 to 1.8 ms for 3700 - 6751 line file that
    # is 180 to 208 KB, which is acceptable for now
    if hasattr(vim, "copy_buf"):
        oc_core_eval("buffer_contents_follow 2")
        p = str(vim.copy_buf())
        return oc_core_eval(p)
    else:
        oc_core_eval("buffer_contents_follow 1")
        return oc_core_eval('\n'.join(vim.current.buffer))

def oc_prune():
    return oc_core_eval('prune 1')

def oc_prune_buffers():
    valid_buffers = []
    for buffer in vim.buffers:
        num = str(buffer.number)
        if vim.eval('buflisted(' + num + ')') == '0':
            continue
        valid_buffers.append(num)

    active_buffer_list = ','.join(valid_buffers)

    return oc_core_eval('prune_buffers ' + active_buffer_list)

def oc_update_current_buffer_info():
    # let server know what is the current buffer
    b = vim.current.buffer
    oc_core_eval('current_buffer_id ' + str(b.number))
    # send server the contents of the current line the cursor is at
    oc_core_eval('current_line ' + vim.current.line)
    # tell server what the current cursor position is
    w = vim.current.window
    cursor_pos = str(w.cursor[0]) + ' ' + str(w.cursor[1])
    oc_core_eval('cursor_position ' + cursor_pos)

    # send current directory to server in preparation for sending tags
    oc_core_eval("current_directory " + vim.eval('getcwd()'))
    # send tags we are using to the server
    oc_core_eval("current_tags " + vim.eval("&tags"))

def oc_get_word_begin_index():
    oc_word_begin_index = oc_eval('get_word_begin_index current_buffer')
    vim.command('let s:completion_begin_col=' + oc_word_begin_index)

def oc_compute_popup_list():
    begin = int(vim.eval("s:completion_begin_col"))
    end = vim.current.window.cursor[1]
    word = vim.current.line[begin:end]
    oc_server_result = oc_eval('complete ' + word)
    if len(oc_server_result) == 0:
        vim.command('let s:completions=[]')
    else:
        vim.command('let s:completions=' + oc_server_result)

    is_corrections_only = oc_eval("is_corrections_only ?")
    if is_corrections_only == '0':
        vim.command('let s:is_corrections_only = 0')
    else:
        vim.command('let s:is_corrections_only = 1')

def oc_taglist():
    if oc_core_plugin_load_exception:
        return
    # send current directory to server in preparation for sending tags
    current_directory = vim.eval("getcwd()")
    oc_eval("current_directory " + current_directory)

    # send current tags we are using to the server
    current_tags = vim.eval("&tags")
    oc_eval("taglist_tags " + current_tags)

    expr = vim.eval("a:expr")
    results = oc_eval("vim_taglist_function " + expr)
    if len(results) == 0:
        return
    vim.command("let l:taglist_results=" + results)
