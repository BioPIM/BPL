# Creation of an empty BPL project

It is possible to add a target that will create an independant BPL project.

You can use the variable `NEWPROJECT_NAME` to define the name of the project and then call CMake with

```
cmake -DNEWPROJECT_NAME=Foo..
```

Then a new target is available and its name will looks like `create_bpl_project_Foo`. Calling `make create_bpl_project_Foo`
will thus create a tar.gz file holding all the necessary resources for building a BPL based program.