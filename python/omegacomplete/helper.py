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

def oc_get_current_line():
    return vim.current.line

def oc_get_cursor_pos():
    w = vim.current.window
    return str(w.cursor[0]) + ' ' + str(w.cursor[1])

def oc_send_current_buffer():
    # this approach takes around 0.8 to 1.8 ms for 3700 - 6751 line file that
    # is 180 to 208 KB, which is acceptable for now
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
