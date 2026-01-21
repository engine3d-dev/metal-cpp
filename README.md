# Library Template

This templated project is used for creating library projects that may be used.

This template is setup to using C++ modules.


# How to Build


Before proceeding, make sure to have done the [getting_started](https://engine3d-dev.github.io/0.1/getting_started/).


## Building the project.

As soon the development environment for Conan is setup.

You should be able to build this project with the following commands:

```bash
conan atlas build . -s build_type=Debug
```

## Building Demos

Your demos will expect the library template to be in conan cache.

Before building demos do:

```
conan atlas create . -s build_type=Debug
```

Then to build demos continue with the following commands:

```
conan atlas build demos -s build_type=Debug
```


To execute the demo:

```
./demos/build/Debug/demo
```
