#include "stdafx.hpp"
#include "Omegacomplete.hpp"

#ifdef _WIN32
#define HAVE_ROUND
#endif
#include <Python.h>

static Omegacomplete* omegacomplete = NULL;

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyObject* eval(PyObject* self, PyObject* args) {
  (void)self;

  const char* input;
  int len;
  if (!PyArg_ParseTuple(args, "s#", &input, &len)) {
    return NULL;
  }
#ifdef OMEGACOMPLETE_TRACE_CMDS
  {
    std::ofstream of("cmd_log.txt", std::ofstream::app);
    of << std::string(input, len) << "\n";
    of.close();
  }
#endif

  const std::string& result = omegacomplete->Eval(input, len);
  return Py_BuildValue("s", result.c_str());
}

static PyMethodDef CoreMethods[] = {
    {"eval", eval, METH_VARARGS,
     "Send a message to the completion core and possibly receive a response"},
    {NULL, NULL, 0, NULL}};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	"core",
	NULL,
	sizeof(struct module_state),
	CoreMethods,
	NULL,
};

PyMODINIT_FUNC
PyInit_core(void)
#else
PyMODINIT_FUNC
initcore(void)
#endif
{
  Omegacomplete::InitStatic();
  omegacomplete = new Omegacomplete;

#if PY_MAJOR_VERSION >= 3
  return PyModule_Create(&module_def);
#else
  (void)Py_InitModule("core", CoreMethods);
#endif
}
#ifdef OMEGACOMPLETE_TRACE_CMDS
#undef OMEGACOMPLETE_TRACE_CMDS
#endif
