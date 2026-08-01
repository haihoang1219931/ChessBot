#include <stdio.h>

typedef struct {
    double x;
    double y;
} Point;

// Function to calculate the center of any cell (row, col) from 0 to 7
Point calculate_cell_center(int row, int col, Point c00, Point c70, Point c77, Point c07) {
    Point center;

    // Normalize coordinates to a 0.0 to 1.0 range
    double u = (double)row / 7.0;
    double v = (double)col / 7.0;

    // Bilinear interpolation formula
    center.x = (1.0 - u) * (1.0 - v) * c00.x +
               u * (1.0 - v) * c70.x +
               u * v * c77.x +
               (1.0 - u) * v * c07.x;

    center.y = (1.0 - u) * (1.0 - v) * c00.y +
               u * (1.0 - v) * c70.y +
               u * v * c77.y +
               (1.0 - u) * v * c07.y;

    return center;
}

int main() {
    // Define your 4 known exact corner centers here
    Point c00 = {680,900};     // Row 0, Col 0
    Point c70 = {-1750,1000};    // Row 7, Col 0
    Point c77 = {-1800,3480};  // Row 7, Col 7
    Point c07 = {650,3450};    // Row 0, Col 7

    // Compute and print centers for all cells
    printf("Row, Col -> (X, Y)\n");
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Point center = calculate_cell_center(r, c, c00, c70, c77, c07);
            printf("Cell [%d][%d] -> (%.2f, %.2f)\n", r, c, center.x, center.y);
        }
    }
    return 0;
}
