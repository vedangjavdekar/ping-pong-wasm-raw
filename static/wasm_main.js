'use strict';

let app = document.getElementById("app");
let ctx = app.getContext("2d");
let wasm = null;

function color_hex(color) {
    const r = ((color>>(0*8))&0xFF).toString(16).padStart(2, '0');
    const g = ((color>>(1*8))&0xFF).toString(16).padStart(2, '0');
    const b = ((color>>(2*8))&0xFF).toString(16).padStart(2, '0');
    const a = ((color>>(3*8))&0xFF).toString(16).padStart(2, '0');
    return "#"+r+g+b+a;
}

function cstrlen(mem, ptr) {
    let len = 0;
    while (mem[ptr] != 0) {
        len++;
        ptr++;
    }
    return len;
}

function cstr_by_ptr(mem_buffer, ptr) {
    const mem = new Uint8Array(mem_buffer);
    const len = cstrlen(mem, ptr);
    const bytes = new Uint8Array(mem_buffer, ptr, len);
    return new TextDecoder().decode(bytes);
}

function platform_log(message_ptr) {
	const buffer = wasm.instance.exports.memory.buffer;
    const message = cstr_by_ptr(buffer, message_ptr);
    console.log(message);
}

function platform_stroke_line_t(x1, y1, x2, y2, thickness, color) {
	prev_lineWidth = ctx.lineWidth;
	ctx.lineWidth = thickness;
	ctx.strokeStyle = color_hex(color);
	ctx.moveTo(x1, y1);
	ctx.lineTo(x2, y2);
	ctx.stroke();
	ctx.lineWidth = prev_lineWidth;
}

function platform_stroke_line(x1, y1, x2, y2, color) {
	ctx.strokeStyle = color_hex(color);
	ctx.moveTo(x1, y1);
	ctx.lineTo(x2, y2);
	ctx.stroke();
}

function platform_stroke_rect(x, y, w, h, color) {
    ctx.strokeStyle = color_hex(color); 
    ctx.strokeRect(x, y, w, h);
}

function platform_fill_rect(x, y, w, h, color) {
    ctx.fillStyle = color_hex(color); 
    ctx.fillRect(x, y, w, h);
}

function platform_stroke_circle(x, y, r, color) {
	ctx.beginPath();
	ctx.arc(x, y, r, 0, 2 * Math.PI);
    ctx.strokeStyle = color_hex(color); 
	ctx.stroke();
}

function platform_fill_circle(x, y, r, color) {
	ctx.beginPath();
	ctx.arc(x, y, r, 0, 2 * Math.PI);
    ctx.fillStyle = color_hex(color); 
	ctx.fill();
}

const PHYSICS_RATE_HZ = 60;
const FIXED_DT = 1.0 / PHYSICS_RATE_HZ;

let prev_time;
let accumulator = 0;
let alpha = 0;

function loop(timestamp) {
	if (prev_time === undefined) {
		prev_time = timestamp;
	}

	const dt = 0.001 * (timestamp - prev_time);
	prev_time = timestamp;

	accumulator += dt;
	while(accumulator >= FIXED_DT) {
		wasm.instance.exports.game_update(FIXED_DT);
		accumulator -= FIXED_DT;
	}
	
	alpha = accumulator / FIXED_DT;

	wasm.instance.exports.game_render(dt, alpha);
    window.requestAnimationFrame(loop);
}

WebAssembly.instantiateStreaming(fetch('./game.wasm'), {
    env: {
		platform_stroke_line_t,
		platform_stroke_line,
		platform_stroke_rect,
		platform_fill_rect,
		platform_stroke_circle,
		platform_fill_circle,
		platform_log
    }
}).then((w) => {
	console.log("Wasm Module Loaded!");
    wasm = w;
    wasm.instance.exports.game_init(app.width, app.height);

	console.log(app);
	console.log(ctx);
    document.addEventListener('keydown', (e) => {
		wasm.instance.exports.game_keydown(e.keyCode);
    });

    document.addEventListener('keyup', (e) => {
		wasm.instance.exports.game_keyup(e.keyCode);
    });

    window.requestAnimationFrame(loop);
});
