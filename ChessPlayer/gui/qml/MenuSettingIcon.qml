import QtQuick 2.0

Canvas {
    id: gearCanvas
    width: 450
    height: 450
    antialiasing: true

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.scale(1.5, 1.5);

        var centerX = 150;
        var centerY = 150;
        var outerR = 130;
        var innerR = 100;
        var holeR = 65;
        var teeth = 7; // Reduced for smoother curves
        var points = [];

        // 1. Draw Rounded Gear Outline
        ctx.beginPath();
        for (var i = 0; i < teeth * 2+1; i++) {
            var angle = (i * Math.PI) / teeth;
            var nextAngle = ((i + 1) * Math.PI) / teeth;

            // Current point
            var r = (i % 2 === 0) ? outerR : innerR;
            var px = centerX + r * Math.cos(angle);
            var py = centerY + r * Math.sin(angle);

            // Midpoint for the curve target
            var nr = ((i + 1) % 2 === 0) ? outerR : innerR;
            var npx = centerX + nr * Math.cos(nextAngle);
            var npy = centerY + nr * Math.sin(nextAngle);

            var midX = (px + npx) / 2;
            var midY = (py + npy) / 2;

            if (i === 0) ctx.moveTo(midX, midY);

            // Draw curve toward the point, ending at the next midpoint
            ctx.quadraticCurveTo(px, py, midX, midY);

            // Store points for the internal wireframe
            points.push({x: px, y: py});
            points.push({x: midX, y: midY});
        }
        ctx.closePath();

        // 2. Inner Hole (Rounded cutout)
        ctx.moveTo(centerX + holeR, centerY);
        ctx.arc(centerX, centerY, holeR, 0, Math.PI * 2, true);

        ctx.strokeStyle = "#4488ff";
        ctx.lineWidth = 1;
        ctx.stroke();

        // 3. Dense Wireframe "Web" (Simulating the image network)
        // Add random nodes strictly within the gear body
        for (var j = 0; j < 60; j++) {
            var a = Math.random() * Math.PI * 2;
            var dist = holeR + 5 + Math.random() * (innerR - holeR - 10);
            points.push({
                x: centerX + dist * Math.cos(a),
                y: centerY + dist * Math.sin(a)
            });
        }

        // Draw the connecting web
        ctx.beginPath();
        ctx.lineWidth = 0.5;
        ctx.strokeStyle = "rgba(0, 204, 255, 0.3)";
        for (var i = 0; i < points.length; i++) {
            for (var k = i + 1; k < points.length; k++) {
                var d = Math.sqrt(Math.pow(points[i].x-points[k].x, 2) + Math.pow(points[i].y-points[k].y, 2));
                if (d < 45) { // Only connect nearby points
                    ctx.moveTo(points[i].x, points[i].y);
                    ctx.lineTo(points[k].x, points[k].y);
                }
            }
        }
        ctx.stroke();

        // 4. Glowing Joint Nodes
        ctx.fillStyle = "#ffffff";
        ctx.shadowBlur = 8;
        ctx.shadowColor = "#00ccff";
        points.forEach(function(p) {
            ctx.beginPath();
            ctx.arc(p.x, p.y, 1.2, 0, Math.PI * 2);
            ctx.fill();
        });
    }
}
