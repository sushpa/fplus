## F+ Markup Language (FML, really).


``` python
# Comments go after a single hash sign and a space.
```

```vb
# Nodes can have tags, like YAML
# Indentation determines structure.
x as TagName =
    b = 43
    k = 22
    d = "Strings must be quoted"
```

```vb
# You can annotate types, linter rules still apply
y[] as String =
    "James"
    "Jock"
    "Whack"
    "More", "On", "Oneline"
```

```vb
bmamr[2] = [
    g = 43,
    k = 12;
    f = 33, n = 33]
```

- No string interpolation and escape codes (data is not static anymore then).
- Strings can be multiline, usual F+ rules apply
- Base indentation is subtracted from multiline strings


```vb
# 2D array literals, just like F+ code.
mat[:,:] = [ 1, 2, 3; 4, 5, 6; 7, 8, 9]
```

```vb
# Units, just like F+ code.
vel[3] as ~m/s = [3.0, 4.0, 5.0]
```

```vb
# Complex number literals
z = 3+4i
```