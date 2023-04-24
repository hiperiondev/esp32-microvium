unit ['script.mvm.js'];

entry ['#entry'];

external console from free-variable 'console';
external vmImport from free-variable 'vmImport';
external vmExport from free-variable 'vmExport';

global thisModule;

function ['#entry']() {
  entry:
    LoadArg(index 0);                    // 1                 script.mvm.js:1:1
    StoreGlobal(name 'thisModule');      // 0                 script.mvm.js:1:1
    Literal(lit &function sayHello);     // 1                 script.mvm.js:1:1
    // script.mvm.js
    LoadGlobal(name 'vmImport');         // 2                 script.mvm.js:2:15
    Literal(lit undefined);              // 3                 script.mvm.js:2:15
    Literal(lit 1);                      // 4                 script.mvm.js:2:24
    Call(count 2);                       // 2                 script.mvm.js:2:24
    LoadGlobal(name 'console');          // 3                 script.mvm.js:2:1
    Literal(lit 'log');                  // 4                 script.mvm.js:2:1
    LoadVar(index 1);                    // 5                 script.mvm.js:2:1
    ObjectSet();                         // 2                 script.mvm.js:2:1
    Pop(count 1);                        // 1                 script.mvm.js:2:1
    LoadGlobal(name 'vmExport');         // 2                 script.mvm.js:6:1
    Literal(lit undefined);              // 3                 script.mvm.js:6:1
    Literal(lit 1234);                   // 4                 script.mvm.js:6:10
    LoadVar(index 0);                    // 5  sayHello       script.mvm.js:6:16
    Call(count 3);                       // 2                 script.mvm.js:6:16
    Pop(count 1);                        // 1                 script.mvm.js:6:16
    Literal(lit undefined);              // 2                 script.mvm.js:6:16
    Return();                            // 1                 script.mvm.js:6:16
}

function sayHello() {
  entry:
    LoadGlobal(name 'console');          // 1                 script.mvm.js:4:3
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