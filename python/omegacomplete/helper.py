# these are probably global to the VIM Python interpreter, so prefix with "oc_"
# to prevent conflicts with any other Python plugins

oc_is_disabled = False

try:
    from omegacomplete.core import eval as oc_core_eval
except:
    oc_is_disabled = True

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
    row = str(vim.current.window.cursor[0])
    col = str(vim.current.window.cursor[1])
    return row + ' ' + col

def oc_send_current_buffer():
    global oc_is_disabled
    if oc_is_disabled:
        return

    header = "buffer_contents_follow 1"
    oc_core_eval(header)
    buffer_contents = '\n'.join(vim.eval("getline(1, '$')"))
    return oc_core_eval(buffer_contents)
