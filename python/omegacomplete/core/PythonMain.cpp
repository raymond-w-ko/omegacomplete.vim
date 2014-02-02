#include "stdafx.hpp"
#include "Omegacomplete.hpp"

static Omegacomplete* omegacomplete = NULL;

static PyObject* eval(PyObject* self, PyObject* args) {
  (void)self;

  const char* input;
  int len;
  if (!PyArg_ParseTuple(args, "s#", &input, &len)) {
    return NULL;
  }

  const std::string& result = omegacomplete->Eval(input, len);
  return Py_BuildValue("s", result.c_str());
}

static PyMethodDef CoreMethods[] = {
  {
    "eval", eval,
    METH_VARARGS,
    "Send a message to the completion core and possibly receive a response"
  },
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initcore(void) {
  (void) Py_InitModule("core", CoreMethods);

  Omegacomplete::InitStatic();
  omegacomplete = new Omegacomplete;
}
