export 1234 = &function sayHello;

slot ['global:console'] = &allocation 6;

function Array_push() {
  entry:
    LoadArg(index 1);                    // 1  value          <builtin>:7:23
    LoadArg(index 0);                    // 2  this           <builtin>:7:3
    LoadArg(index 0);                    // 3  this           <builtin>:7:8
    Literal(lit 'length');               // 4                 <builtin>:7:8
    ObjectGet();                         // 3                 <builtin>:7:8
    LoadVar(index 0);                    // 4                 <builtin>:7:8
    ObjectSet();                         // 1                 <builtin>:7:8
    Pop(count 1);                        // 0                 <builtin>:7:8
    Literal(lit undefined);              // 1                 <builtin>:8:2
    Return();                            // 0                 <builtin>:8:2
}

function sayHello() {
  entry:
    LoadGlobal(name 'global:console');   // 1                 script.mvm.js:4:3
    LoadVar(index 0);                    // 2                 script.mvm.js:4:3
    Literal(lit 'log');                  // 3                 script.mvm.js:4:3
    ObjectGet();                         // 2                 script.mvm.js:4:3
    LoadVar(index 0);                    // 3                 script.mvm.js:4:3
    Literal(lit 'Hello, World!');        // 4                 script.mvm.js:4:15
    Call(count 2);                       // 2                 script.mvm.js:4:15
    StoreVar(index 0);                   // 1                 script.mvm.js:4:15
    Pop(count 1);                        // 0                 script.mvm.js:4:15
    Literal(lit undefined);              // 1                 script.mvm.js:5:2
    Return();                            // 0                 script.mvm.js:5:2
}

allocation 5 = {
  push: &function Array_push,
};

allocation 6 = {
  log: host function 1,
};