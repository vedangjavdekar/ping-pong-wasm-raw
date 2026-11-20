import PingPong from "../../wasm/main.js"

let module = null
const moduleFns = {
  init: (x: number, y: number) => { },
  player1_key_event: (isUp: boolean, isDown: boolean,) => { },
  player2_key_event: (isUp: boolean, isDown: boolean,) => { },
}

onmessage = async function (event: any) {
  const { action, data } = event.data;
  switch (action) {
    case 'init': {
      try {
        if (module) {
          console.log('game alread initialised',);
          return;
        }
        module = await PingPong();
        moduleFns.init = module?.cwrap('init', '', ['number', 'number']);
        moduleFns.player1_key_event = module?.cwrap('player1_key_event', '', ['boolean', 'boolean']);
        moduleFns.player2_key_event = module?.cwrap('player2_key_event', '', ['boolean', 'boolean']);

        moduleFns.init(800, 600);
        console.log('starting another one')

      } catch (e) {
        console.log('error', e);
      }
      break;
    }
    case 'p1_event': {
      try {
        moduleFns.player1_key_event(data.isUp, data.isDown);
      } catch (e) {
        console.log('p1 error', e);
      }
      break;
    }
    case 'p2_event': {
      try {
        moduleFns.player2_key_event(data.isUp, data.isDown);
      } catch (e) {
        console.log('p2 error', e);
      }
      break;
    }
  }
  postMessage("donee")
}
