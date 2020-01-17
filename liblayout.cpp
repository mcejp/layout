#include <numeric>
#include <gsl/span>
#include <vector>

#include <cstdio>

using gsl::span;
using std::max;
using std::min;
using std::vector;

#define assert(x_) if (!(x_)) { fprintf(stderr, "ASSERTION ERROR [%s:%d]: %s doesn't hold\n", __FILE__, __LINE__, #x_); }

[[noreturn]] void LayoutError(const char* message) {
    fprintf(stderr, "shit %s\n", message);
    exit(-1);
}

// Universal layout algorithm for a sequence of items in one direction
vector<int> Compute1DLayout(int availableSpace,
                            span<const int> minSizes,
                            span<const int> maxSizes,
                            int sumMinSizes,
                            bool expand) {
    int numRows = minSizes.size();
    int extraSpace = availableSpace - sumMinSizes;

    if (extraSpace < 0 || !expand) {
        extraSpace = 0;
    }

    int totalSpace = sumMinSizes + extraSpace;

    // distribute any extra space if permitted
    std::vector<bool> rowIsFixed;
    rowIsFixed.resize(numRows, false);
    std::vector<int> rowHeights { minSizes.begin(), minSizes.end()};

    for (; extraSpace > 0;) {
        // one round of iteration
        // - determine total height of fixed rows
        // - determine number of free rows and space available to them
        // - expand free rows, turn into fixed where appropriate
        // - iterate until set of fixed rows is constant

        int heightInFixedRows = 0;
        int numExpandableRows = 0;

        for (int row = 0; row < numRows; row++) {
            if (rowIsFixed[row]) {
                heightInFixedRows += rowHeights[row];
            }
            else {
                numExpandableRows++;
            }
        }

        int spaceAvailableToExpandableRows = totalSpace - heightInFixedRows;
        int spaceUsedByExpandableRows = 0;
        bool newRowsFixed = false;
        int indexOfExpandableRow = 0;

        for (int row = 0; row < numRows; row++) {
            if (rowIsFixed[row]) {
                continue;
            }
            else {
                // evenly distribute remaining space
                // (in a way that prevents accumulation of integer division rounding error)
                int spaceAvailable = (spaceAvailableToExpandableRows * (indexOfExpandableRow + 1) / numExpandableRows) - spaceUsedByExpandableRows;

                if (spaceAvailable > maxSizes[row]) {
                    rowHeights[row] = maxSizes[row];
                    rowIsFixed[row] = true;
                    newRowsFixed = true;
                }
                else {
                    rowHeights[row] = spaceAvailable;
                }

                spaceUsedByExpandableRows += rowHeights[row];
                indexOfExpandableRow++;
            }
        }

        // no more changes, we can bail
        if (!newRowsFixed) {
            break;
        }
    }

    return rowHeights;
}

std::tuple<vector<int>, vector<int>> Compute2DLayout(int numColumns,
                                                     int availableSpaceX,
                                                     int availableSpaceY,
                                                     span<const int> widgetMinSizesX,
                                                     span<const int> widgetMinSizesY,
                                                     span<const int> widgetMaxSizesX,
                                                     span<const int> widgetMaxSizesY,
                                                     int cellPaddingX,
                                                     int cellPaddingY,
                                                     bool expandX,
                                                     bool expandY) {
    assert(widgetMinSizesX.size() == widgetMinSizesY.size());
    assert(widgetMaxSizesX.size() == widgetMaxSizesY.size());
    assert(widgetMinSizesX.size() == widgetMaxSizesX.size());

    int numWidgets = widgetMinSizesY.size();

    if (numWidgets == 0) {
        LayoutError("empty");
    }

    int numRows = (numWidgets + numColumns - 1) / numColumns;
    assert(numRows >= 1);

    // horizontal lay-out
    // ================

    // determine minimum & maximum column widths

    vector<int> colMinWidths, colMaxWidths;
    int totalMinWidth = 0;

    for (int col = 0; col < numColumns; col++) {
        auto widget0MinWidth = widgetMinSizesX[col] + cellPaddingX * 2;
        auto widget0MaxWidth = widgetMaxSizesX[col] + cellPaddingX * 2;

        int colMinWidth = widget0MinWidth;
        int colMaxWidth = widget0MaxWidth;

        for (int row = 1; row < numRows && row * numColumns + col < numWidgets; row++) {
            int widgetMinWidth = widgetMinSizesX[row * numColumns + col] + cellPaddingX * 2;
            int widgetMaxWidth = widgetMaxSizesX[row * numColumns + col] + cellPaddingX * 2;

            colMinWidth = max(colMinWidth, widgetMinWidth);
            colMaxWidth = min(colMaxWidth, widgetMaxWidth);
        }

        if (colMaxWidth < colMinWidth) {
            LayoutError("Incompatible layout");
        }

        colMinWidths.push_back(colMinWidth);
        colMaxWidths.push_back(colMaxWidth);
        totalMinWidth += colMinWidth;
    }

    auto colWidths = Compute1DLayout(availableSpaceX, colMinWidths, colMaxWidths, totalMinWidth, expandX);

    // vertical lay-out
    // ================

    // determine minimum & maximum row heights

    vector<int> rowMinHeights, rowMaxHeights;
    int totalMinHeight = 0;

    for (int row = 0; row < numRows; row++) {
        auto widget0MinHeight = widgetMinSizesY[row * numColumns + 0] + cellPaddingY * 2;
        auto widget0MaxHeight = widgetMaxSizesY[row * numColumns + 0] + cellPaddingY * 2;

        int rowMinHeight = widget0MinHeight;
        int rowMaxHeight = widget0MaxHeight;

        for (int col = 1; col < numColumns && row * numColumns + col < numWidgets; col++) {
            int widgetMinHeight = widgetMinSizesY[row * numColumns + col] + cellPaddingY * 2;
            int widgetMaxHeight = widgetMaxSizesY[row * numColumns + col] + cellPaddingY * 2;

            rowMinHeight = max(rowMinHeight, widgetMinHeight);
            rowMaxHeight = min(rowMaxHeight, widgetMaxHeight);
        }

        if (rowMaxHeight < rowMinHeight) {
            LayoutError("Incompatible layout");
        }

        rowMinHeights.push_back(rowMinHeight);
        rowMaxHeights.push_back(rowMaxHeight);
        totalMinHeight += rowMinHeight;
    }

    auto rowHeights = Compute1DLayout(availableSpaceY, rowMinHeights, rowMaxHeights, totalMinHeight, expandY);

    return { colWidths, rowHeights };
}

auto TestCompute1DLayout(int areaH, span<int> rowMinHeights, span<int> rowMaxHeights, bool expandVert) {
    int numRows = rowMinHeights.size();
    printf("Laying out into %d px:\n", areaH);
    for (int row = 0; row < rowMinHeights.size(); row++) {
        printf("  -- %9d .. %9d px\n", rowMinHeights[row], rowMaxHeights[row]);
    }

    vector<int> rowHeights = Compute1DLayout(areaH, rowMinHeights, rowMaxHeights, std::accumulate(rowMinHeights.cbegin(), rowMinHeights.cend(), 0), expandVert);

    printf("\n\nCalculated:\n");
    for (int row = 0; row < rowHeights.size(); row++) {
        printf("  -- %9d px\n", rowHeights[row]);
    }

    int sumRowHeights = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
    printf("Using: %d px\n", sumRowHeights);

    return rowHeights;
}

void main() {
    {
        // tc1
        int rowMinHeights[] = { 10, 15, 10 };
        int rowMaxHeights[] = { 10, INT_MAX, 10 };
        auto rowHeights = TestCompute1DLayout(100, rowMinHeights, rowMaxHeights, true);

        assert(rowHeights[0] == 10);
        assert(rowHeights[1] == 80);
        assert(rowHeights[2] == 10);
    }

    {
        // tc2
        int rowMinHeights[] = { 10, 15, 10 };
        int rowMaxHeights[] = { 10, 20, 10 };
        auto rowHeights = TestCompute1DLayout(100, rowMinHeights, rowMaxHeights, true);

        assert(rowHeights[0] == 10);
        assert(rowHeights[1] == 20);
        assert(rowHeights[2] == 10);
    }

    {
        // tc3
        int rowMinHeights[] = { 10, 15, 10 };
        int rowMaxHeights[] = { 10, 20, 10 };
        auto rowHeights = TestCompute1DLayout(100, rowMinHeights, rowMaxHeights, false);

        assert(rowHeights[0] == 10);
        assert(rowHeights[1] == 15);
        assert(rowHeights[2] == 10);
    }

    {
        // tc4
        int rowMinHeights[] = { 10, 10, 10, 10, 10 };
        int rowMaxHeights[] = { 10, 25, INT_MAX, INT_MAX, 10 };
        auto rowHeights = TestCompute1DLayout(100, rowMinHeights, rowMaxHeights, true);

        assert(rowHeights[0] == 10);
        assert(rowHeights[1] == 80);
        assert(rowHeights[2] == 10);
    }
}
