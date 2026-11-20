<script lang="ts">
    import type { Action } from 'svelte/action';
    import { onMount } from 'svelte';

    let loading = $state(true)
    let error = $state('')
    let workerRef = $state<Worker | null>()
    let frameRef = $state<HTMLCanvasElement | null>()
    let pressedKeys = $state(new Map<number, boolean>())

    const width = 800;
    const height = 600;

    const updateGameKeys = () => {
      workerRef?.postMessage({
        action: 'p1_event',
        data: {
          isUp: pressedKeys.get(87),
          isDown: pressedKeys.get(83),
        }
      })

      workerRef?.postMessage({
        action: 'p2_event',
        data: {
          isUp: pressedKeys.get(38),
          isDown: pressedKeys.get(40),
        }
      })
    }

    const onPlayerKeyUp = (event: KeyboardEvent) => {
      const kc = event.keyCode;
      if (kc === 87) pressedKeys.set(87, false);
      if (kc === 83) pressedKeys.set(83, false);
      if (kc === 38) pressedKeys.set(38, false);
      if (kc === 40) pressedKeys.set(40, false);
      updateGameKeys();
    }

    const onPlayerKeyDown = (event: KeyboardEvent) => {
      const kc = event.keyCode;
      if (kc === 87) pressedKeys.set(87, true);
      if (kc === 83) pressedKeys.set(83, true);
      if (kc === 38) pressedKeys.set(38, true);
      if (kc === 40) pressedKeys.set(40, true);
      updateGameKeys();
    }

    onMount(() => {
      try {
        setInterval(() => {}, 200)
        const worker = new Worker(
            new URL("../lib/workers/looper", import.meta.url),
            { type: "module" }
        );

        if (!worker) {
            console.log('failed to load game worker', );
            error = 'failed to load game worker'
            return;
        }

        worker.addEventListener('error', (event) => {
          console.error('Worker failed:', event.message);
        })

        worker.addEventListener('message', ({data: {action, data}}) => {
          if (action === 'update_frame') {
            const ctx = frameRef?.getContext('2d');

            const imageData = new ImageData(
                new Uint8ClampedArray(data),
                width,
                height
            );

            ctx?.putImageData(imageData, 0, 0);
          }

          if (action === 'debug') {
            console.log('debug', data)
          }
        })
        workerRef = worker
        loading = false
      } catch (e) {
        console.log('error', e);
      }
    })
    const initDrawBox: Action = (node: any) => {
        const ctx = node.getContext('2d');
        ctx.imageSmoothingEnabled = true;
    }

    const handleGameStart = () => {
        console.log('do something', workerRef);
        try {
          workerRef?.postMessage({action: 'init'})
        } catch (e) {
          console.log('error', e);
        }
    }

</script>

<svelte:window onkeydown={onPlayerKeyDown} onkeyup={onPlayerKeyUp} />
<section class="p-2 bg-gray-400">
    {#if loading}
        <span>loading module</span>
  {:else if error}
      <span>error {error}</span>
    {:else}
        <section class="flex flex-col gap-1" >
            <button type='button' class="w-50 bg-red-600 p-1 rounded justify-start cursor-pointer" onclick={handleGameStart}>start game</button>
        </section>
        <canvas id="drawBox" class="m-5" height={height} width={width} bind:this={frameRef} use:initDrawBox ></canvas>
    {/if}
</section>

<style>
    #drawBox {
        image-rendering: pixelated;
    }
</style>
