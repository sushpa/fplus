
### Run tests

Run all tests in the project:
```python
f+ test
```
This tests each module that is a dependency of one of the programs in the project.

*If a module exists that is not used by any program, it is marked as an unused module and will not be tested, as long as it is unused.*

Run the tests in a module:

```python
f+ test mymod
f+ test modules/mymod
f+ test modules/mymod.fp
```

Run a particular test:
```python
f+ test mymod "should handle negative values"
```

Recall that all names are generally case-insensitive, and so are test descriptions. Modules are defined starting with a lower-case letter, although usages such as `f+ test Mymod` will still work, since it can  find the right module. In much the same way, a variable `foo` can be referred to as `Foo` in the code and it will be resolved (and cleaned up by the linter).

### Test reports

After tests have been run, you see the following report if everything went OK:

```python
----------------------------------------------------
 Tests       Pass       Fail       Skip         Time
====================================================
   321        321          0          0       3m 12s
----------------------------------------------------
```

In case of failures:

```python
 TEST FAILED
----------------------------------------------------
 simptest.fp:23:
    test failed: "should handle negative values" #1344

 TEST FAILED
----------------------------------------------------
 simptest.fp:66:
    test failed: "should invert null parameters"

----------------------------------------------------
 Tests       Pass       Fail       Skip         Time
====================================================
   321        319          2          0       3m 46s
----------------------------------------------------
```