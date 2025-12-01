# C++ Object Constructor Failure Affect on Destructor

We test if the destructor of a C++ object that failed construction will run.

## Result

```
A: C++ constructor
Memory allocation failed: bad allocation
```

No, the destructor of an object that failed construction will not run. The object's destructor is only run when it is deleted from a fully initialized state. This finding applies no matter what scope the object exists in.
