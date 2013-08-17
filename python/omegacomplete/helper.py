# these are probably global to the VIM Python interpreter, so prefix with "oc_"
# to prevent conflicts with any other Python plugins

oc_is_disabled = False

try:
    from omegacomplete.core import eval as oc_core_eval
except Exception as e:
    oc_is_disabled = True
    print(e)

def oc_eval(cmd):
    global oc_is_disabled
    if oc_is_disabled:
        return

    try:
        return oc_core_eval(cmd)
    except:
        oc_is_disabled = True

def oc_get_current_buffer_contents():
    global oc_is_disabled
    if oc_is_disabled:
        return ""

    return "\n".join(vim.current.buffer)

def oc_get_current_line():
    global oc_is_disabled
    if oc_is_disabled:
        return ""

    return vim.current.line

def oc_disable_check():
    global oc_is_disabled
    if oc_is_disabled:
        return '1'
    else:
        return '0'

def oc_get_cursor_pos():
    w = vim.current.window
    return str(w.cursor[0]) + ' ' + str(w.cursor[1])

def oc_send_current_buffer():
    global oc_is_disabled
    if oc_is_disabled:
        return

    # this approach takes around 0.8 to 1.8 ms for 3700 - 6751 line file that
    # is 180 to 208 KB, which is acceptable for my tastes
    header = "buffer_contents_follow 1"
    oc_core_eval(header)
    buffer_contents = '\n'.join(vim.current.buffer)
    ret = oc_core_eval(buffer_contents)

    return ret

def oc_prune_buffers():
    global oc_is_disabled
    if oc_is_disabled:
        return

    valid_buffers = []
    for buffer in vim.buffers:
        num = str(buffer.number)
        if vim.eval('buflisted(' + num + ')') == '0':
            continue
        valid_buffers.append(num)

    active_buffer_list = ','.join(valid_buffers)

    return oc_core_eval('prune_buffers ' + active_buffer_list)
