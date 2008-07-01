var c = CanvasRenderingContext2D(root.lookup("/device/canvas"));

// bar graph
var top = 50;
var bottom = 250;

var width = 20;
var height;
var x;
var y;

c.beginPath();
c.closePath();

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

c.beginPath();
c.closePath();

c.fillStyle = "red";
c.translate(512, 200);
c.mozTextStyle = "36px Italic Liberation Serif";
c.mozDrawText("Hello, world.");
c.translate(-512, -200);
c.fill();

c.fillStyle = "lime";
c.translate(512, 250);
c.mozTextStyle = "40px Bold Liberation Sans";
c.mozDrawText("Hello, world.");
c.translate(-512, -250);
c.fill();

c.fillStyle = "blue";
c.translate(512, 300);
c.mozTextStyle = "48px Liberation Mono";
c.mozDrawText("Hello, world.");
c.translate(-512, -300);
c.fill();

c.fillStyle = "fuchsia";
c.translate(512, 350);
c.mozTextStyle = "48px Sazanami Gothic";
c.mozDrawText("こんにちは、世界。");
c.translate(-512, -350);
c.fill();

c.fillStyle = "aqua";
c.translate(512, 400);
c.mozTextStyle = "48px Sazanami Mincho";
c.mozDrawText("こんにちは、世界。");
c.translate(-512, -400);
c.fill();

