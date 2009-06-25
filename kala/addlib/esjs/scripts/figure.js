var c = CanvasRenderingContext2D(root.lookup("/device/canvas"));

// bar graph
var top = 50;
var bottom = 250;

var width = 20;
var height;
var x;
var y;

c.strokeStyle = "rgb(0, 0, 0)";

height = 100;
x = 100;
c.lineWidth = 3;
c.fillStyle = "rgb(255, 0, 0)";

c.fillRect(x, bottom - height, width, height);

x += 50;
height = 200;
c.fillStyle = "rgb(0, 255, 0)";
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 80;
c.fillStyle = "rgb(0, 0, 255)";
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 50;
c.fillStyle = "rgb(255, 255, 0)";
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 30;
c.fillStyle = "rgb(0, 255, 255)";
c.fillRect(x, bottom - height, width, height);

c.moveTo(80, top);
c.lineTo(80, bottom);
c.lineTo(350, bottom);
c.lineWidth = 4;
c.stroke();

// circle graph
var r = 100;  // radius
var cx = 180; // center
var cy = 450; // center
// angle
var s = 270/180;
var e = 120/180;

c.fillStyle = "rgb(255, 0, 0)";
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 240/180;
c.fillStyle = "rgb(0, 255, 0)";
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 260/180;
c.fillStyle = "rgb(0, 0, 255)";
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 270/180;
c.fillStyle = "rgb(255, 255, 0)";
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

c.fillStyle = "red";
c.font = "36pt Italic Liberation Serif";
c.fillText("Hello, world.", 512, 200);

c.fillStyle = "lime";
c.font = "40pt Bold Liberation Sans";
c.fillText("Hello, world.", 512, 250);

c.fillStyle = "blue";
c.font = "48pt Liberation Mono";
c.fillText("Hello, world.", 512, 300);

c.fillStyle = "fuchsia";
c.font = "48pt Sazanami Gothic";
c.fillText("こんにちは、世界。", 512, 350);

c.fillStyle = "aqua";
c.font = "48pt Sazanami Mincho";
c.fillText("こんにちは、世界。", 512, 400);
