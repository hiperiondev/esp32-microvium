// script.mvm.js

import * as wifi from '../components/microvium-uc-hal/js/microvium_hal_wifi.js';

console.log = vmImport(1);
function sayHello() {
  console.log('Hello, World!');
  wifi.Connect("test", "test1234");
}
vmExport(1234, sayHello);
