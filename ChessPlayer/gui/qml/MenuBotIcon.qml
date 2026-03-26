import QtQuick 2.0

Canvas {
    id: robotCanvas
    width: 450
    height: 450
    antialiasing: true

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.scale(1.5, 1.5); // Global 150% Scale

        var blueGlow = "#00ccff";
        var deepBlue = "#0055ff";
        var allPoints = [];

        // --- Helper: Draw and collect points for wireframe ---
        function drawRoundedPath(points, close) {
            ctx.beginPath();
            ctx.moveTo(points[0].x, points[0].y);
            for (var i = 1; i < points.length - 2; i++) {
                var xc = (points[i].x + points[i + 1].x) / 2;
                var yc = (points[i].y + points[i + 1].y) / 2;
                ctx.quadraticCurveTo(points[i].x, points[i].y, xc, yc);
            }
            // Curve through the last two points
            ctx.quadraticCurveTo(points[i].x, points[i].y, points[i+1].x, points[i+1].y);
            if(close) ctx.closePath();

            ctx.strokeStyle = "rgba(0, 150, 255, 0.5)";
            ctx.lineWidth = 1.5;
            ctx.stroke();
            allPoints = allPoints.concat(points);
        }

        // 1. Head (Rounded Tablet Shape)
        var head = [
            {x: 70, y: 80}, {x: 80, y: 40}, {x: 150, y: 35},
            {x: 220, y: 40}, {x: 230, y: 80}, {x: 220, y: 130},
            {x: 150, y: 140}, {x: 80, y: 130}, {x: 70, y: 80}
        ];
        drawRoundedPath(head, true);

        // 2. Body (Heart/Shield Shape)
        var body = [
            {x: 150, y: 155}, {x: 210, y: 170}, {x: 190, y: 260},
            {x: 150, y: 290}, {x: 110, y: 260}, {x: 90, y: 170}, {x: 150, y: 155}
        ];
        drawRoundedPath(body, true);

        // 3. Arms (Tapered Wings)
        var leftArm = [{x: 85, y: 180}, {x: 30, y: 210}, {x: 50, y: 270}, {x: 85, y: 200}];
        var rightArm = [{x: 215, y: 180}, {x: 270, y: 210}, {x: 250, y: 270}, {x: 215, y: 200}];
        drawRoundedPath(leftArm, true);
        drawRoundedPath(rightArm, true);

        // 4. Internal Wireframe "Web"
        ctx.beginPath();
        ctx.lineWidth = 0.4;
        ctx.strokeStyle = "rgba(0, 204, 255, 0.25)";
        for (var i = 0; i < allPoints.length; i++) {
            for (var k = i + 1; k < allPoints.length; k++) {
                var d = Math.sqrt(Math.pow(allPoints[i].x - allPoints[k].x, 2) + Math.pow(allPoints[i].y - allPoints[k].y, 2));
                // Only connect points within the same body part (approx by distance)
                if (d < 55) {
                    ctx.moveTo(allPoints[i].x, allPoints[i].y);
                    ctx.lineTo(allPoints[k].x, allPoints[k].y);
                }
            }
        }
        ctx.stroke();

        // 5. Glowing Eyes (The highlight of the image)
        function drawGlowingEye(x, y) {
            ctx.save();
            ctx.shadowBlur = 15;
            ctx.shadowColor = blueGlow;

            // Outer Ring
            ctx.beginPath();
            ctx.arc(x, y, 22, 0, Math.PI * 2);
            ctx.strokeStyle = blueGlow;
            ctx.lineWidth = 2;
            ctx.stroke();

            // Bright Inner Ring
            ctx.shadowBlur = 8;
            ctx.beginPath();
            ctx.arc(x, y, 18, 0, Math.PI * 2);
            ctx.strokeStyle = "white";
            ctx.lineWidth = 3;
            ctx.stroke();
            ctx.restore();
        }
        drawGlowingEye(110, 85);
        drawGlowingEye(190, 85);

        // 6. Tiny Node Dots
        ctx.fillStyle = "white";
        allPoints.forEach(function(p) {
            ctx.beginPath();
            ctx.arc(p.x, p.y, 1, 0, Math.PI * 2);
            ctx.fill();
        });
    }
}
