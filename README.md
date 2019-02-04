# laft-multimethod

Multimethods are method that depend on the subtype of two (or more) arguments (where virtual methods depend on one). 

The most common implementation is the visitor pattern.

# Current comparison to visitor pattern
+ = Both support only two types (the visited and the visitor).
+ \+ Support different return values (one can return int and the other can return string for example)
+ \+ Only one indirect function call.
+ \- Less readable error messages.
+ \- Easier to make an error when implementing.

# Possible extensions
It should be possible to extends the functionnality to support any number of arguments.

Exemple: auto point = laft::multimethod::dispatch(Intersect{}, form1, form2);
