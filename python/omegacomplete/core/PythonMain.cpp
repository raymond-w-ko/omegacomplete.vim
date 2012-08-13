#include "stdafx.hpp"
#include "OmegaComplete.hpp"

static PyObject* eval(PyObject* self, PyObject* args)
{
    const char* input;
    int len;
    if (!PyArgs_ParseTuple(args, "s#", &input, &len))
        return NULL;

    const std::string& result = OmegaComplete::GetInstance()->Eval(input, len);
    return Py_BuildValue("s", result.c_str());
}

static PyMethodDef CoreMethods[] =
{
    {
        "eval", eval,
        METH_VARARGS,
        "Send a message to the completion core and possibly receive a response"
    },
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initcore(void)
{
    (void) Py_InitModule("core", CoreMethods);
    OmegaComplete::InitStatic();
}
