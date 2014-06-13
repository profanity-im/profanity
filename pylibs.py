from distutils.sysconfig import get_python_lib
import site

print get_python_lib()

print site.getsitepackages()
