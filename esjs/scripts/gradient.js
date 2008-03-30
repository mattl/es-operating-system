var ctx = CanvasRenderingContext2D(root.lookup("/device/canvas"));

var gradObj = ctx.createLinearGradient(0, 0, 320, 240);
gradObj.addColorStop(0.3, "#ff0000");
gradObj.addColorStop(0.6, "#0000ff");

ctx.fillGradient = CanvasGradient(gradObj);
ctx.rect(10, 10, 300, 220);
ctx.fill();
