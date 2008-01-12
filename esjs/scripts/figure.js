var c = ICanvasRenderingContext2D(root.lookup("/device/canvas"));

// bar graph
var top = 50;
var bottom = 250;

var width = 20;
var height;
var x;
var y;

c.setStrokeStyle("rgba(0, 0, 0, 1)");

height = 100;
x = 100;
c.setLineWidth(3);
c.setFillStyle("rgba(255, 0, 0, 1)");

c.fillRect(x, bottom - height, width, height);

x += 50;
height = 200;
c.setFillStyle("rgba(0, 255, 0, 1)");
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 80;
c.setFillStyle("rgba(0, 0, 255, 1)");
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 50;
c.setFillStyle("rgba(255, 255, 0, 1)");
c.fillRect(x, bottom - height, width, height);

x += 50;
height = 30;
c.setFillStyle("rgba(0, 255, 255, 1)");
c.fillRect(x, bottom - height, width, height);

c.moveTo(80, top);
c.lineTo(80, bottom);
c.lineTo(350, bottom);
c.setLineWidth(4);
c.stroke();

// circle graph
var r = 100;  // radius
var cx = 180; // center
var cy = 450; // center
// angle
var s = 270/180;
var e = 120/180;

c.setFillStyle("rgba(255, 0, 0, 1)");
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 240/180;
c.setFillStyle("rgba(0, 255, 0, 1)");
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 260/180;
c.setFillStyle("rgba(0, 0, 255, 1)");
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();

s = e;
e = 270/180;
c.setFillStyle("rgba(255, 255, 0, 1)");
c.beginPath();
c.arc(cx, cy, r, Math.PI * s, Math.PI * e, 0);
c.lineTo(cx, cy);
c.closePath();
c.fill();
