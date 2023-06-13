#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clickablelabel.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QVector>
#include <QQueue>
#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QPoint>
#include <QColor>
#include <QRgb>
#include <QMouseEvent>
#include <QDebug>
#include <QPaintEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsPixmapItem>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    label_pic2(nullptr) // Initialize the label_pic2 pointer to nullptr
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), tr("Images (*.png *.xpm *.jpg)"));

    if (!file_name.isEmpty())
    {
        // Open prompt and display the image
        QMessageBox::information(this, "...", file_name);
        QImage img(file_name);
        QPixmap pix = QPixmap::fromImage(img);

        // Get label dimensions
        int w = ui->label_pic->width();
        int h = ui->label_pic->height();

        // Load the image into the UI
        ui->label_pic->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio));

        // Get image width and height, create an empty binary matrix
        unsigned int cols = img.width();
        unsigned int rows = img.height();
        unsigned int numBlackPixels = 0;
        QVector<QVector<int>> imgArray(rows, QVector<int>(cols, 0));

        // Get pixel data and update the matrix
        for (unsigned int i = 0; i < rows; i++)
        {
            for (unsigned int j = 0; j < cols; j++)
            {
                // img.pixel(x,y) where x = col, y = row (coordinates)
                QColor clrCurrent(img.pixel(j, i));
                int r = clrCurrent.red();
                int g = clrCurrent.green();
                int b = clrCurrent.blue();
                int a = clrCurrent.alpha();

                // If black, assign 1
                // Black r = 0, g = 0, b = 0, a = 255
                if (r + g + b < 20 && a > 240)
                {
                    imgArray[i][j] = 1;
                    numBlackPixels += 1;
                }
            }
        }

        // Update UI with information
        ui->dims->setText(QString("W: %1  H: %2").arg(cols).arg(rows));
        float pD = (static_cast<float>(numBlackPixels) / (cols * rows)) * 100.0f;
        ui->pDark->setText(QString::number(pD));

        // Image segmentation
        QImage originalImage = img.copy();

        // Create a segmented image with the same dimensions as the original image
        segmentedImage = QImage(originalImage.size(), QImage::Format_ARGB32);

        // Iterate over the pixels of the original image
        for (int y = 0; y < originalImage.height(); ++y)
        {
            for (int x = 0; x < originalImage.width(); ++x)
            {
                QRgb pixelColor = originalImage.pixel(x, y);

                // Perform color-based segmentation
                // Define the segmentation criteria based on the pixel color
                int red = qRed(pixelColor);
                int green = qGreen(pixelColor);
                int blue = qBlue(pixelColor);

                bool isSegment = false;
                // Example segmentation criteria: segment if the sum of RGB components is below a threshold
                int sumThreshold = 550;
                int sum = red + green + blue;
                if (sum < sumThreshold)
                {
                    isSegment = true;
                }

                if (isSegment)
                {
                    // Set the pixel in the segmented image to the original color
                    segmentedImage.setPixel(x, y, pixelColor);
                }
                else
                {
                    // Set the pixel in the segmented image to a different color (e.g., black)
                    segmentedImage.setPixel(x, y, qRgb(0, 0, 0));
                }
            }
        }

        // Load the image into the UI
        QPixmap segmentedPixmap = QPixmap::fromImage(segmentedImage);
        ui->label_pic_2->setPixmap(segmentedPixmap.scaled(w, h, Qt::KeepAspectRatio));


        // Create an instance of ClickableLabel and assign it to label_pic2
        label_pic2 = new ClickableLabel(this);

        // Create a layout and set it as the central widget layout
        QHBoxLayout* layout = new QHBoxLayout(ui->centralwidget);
        layout->addWidget(label_pic2);
        layout->setContentsMargins(80, 280, 20, 20);

        // Connect clicked signal of label_pic2 to slot mousePressedSlot
        connect(label_pic2, &ClickableLabel::mousePressed, this, &MainWindow::mousePressedSlot);
    }
}

void MainWindow::mousePressedSlot(const QPoint& pos)
{
    // Check if the click position is within the boundaries of the segmented image in label_pic_2
    QRect labelRect = label_pic_2.geometry();
    if (!labelRect.contains(pos))
    {
        return;
    }

    // Clear the previously selected segment
    selectedSegment.clear();

    // Add the click position to the selected segment
    selectedSegment.append(pos);

    // Display the click position
    QString pointString = QString("%1, %2").arg(pos.x()).arg(pos.y());
    ui->pDark_2->setText(pointString);

    // Calculate the perimeter coordinates and area of the segment
 //   calculateSegmentProperties(perimeterCoordinates);

    // Update label_pic2 to draw the selected segment
    label_pic2->update();
}

QVector<QPoint> MainWindow::expandSegment(const QColor& color)
{
    QVector<QPoint> perimeterCoordinates;
    QQueue<QPoint> queue;
    for (const QPoint& point : selectedSegment)
    {
        queue.enqueue(point);
    }

    while (!queue.isEmpty())
    {
        QPoint currentPos = queue.dequeue();
        if (selectedSegment.contains(currentPos))
        {
            continue; // Skip if already processed
        }

        // Add the current position to the selected segment
        selectedSegment.append(currentPos);

        // Get the color of the current position
        QColor currentColor = segmentedImage.pixelColor(currentPos);

        // Check if the color matches the clicked color
        if (currentColor == color)
        {
            // Check neighboring pixels (up, down, left, right)
            QPoint neighbors[] = {
                QPoint(currentPos.x(), currentPos.y() - 1), // up
                QPoint(currentPos.x(), currentPos.y() + 1), // down
                QPoint(currentPos.x() - 1, currentPos.y()), // left
                QPoint(currentPos.x() + 1, currentPos.y())  // right
            };

            bool isPerimeter = false;

            for (const QPoint& neighbor : neighbors)
            {
                // Check if the neighbor is within the image boundaries
                if (neighbor.x() >= 0 && neighbor.x() < segmentedImage.width() &&
                    neighbor.y() >= 0 && neighbor.y() < segmentedImage.height())
                {
                    // Check if the neighbor is not already in the selected segment
                    if (!selectedSegment.contains(neighbor))
                    {
                        // Check if the neighbor is a perimeter point
                        QColor neighborColor = segmentedImage.pixelColor(neighbor);
                        if (neighborColor != color)
                        {
                            // Add the perimeter point to the perimeter coordinates
                            perimeterCoordinates.append(neighbor);
                            isPerimeter = true;
                        }

                        // Enqueue the neighbor for processing
                        queue.enqueue(neighbor);
                    }
                }
            }

            if (!isPerimeter)
            {
                // Add the current position to the perimeter coordinates
                perimeterCoordinates.append(currentPos);
            }
        }
    }

    return perimeterCoordinates;
}

void MainWindow::calculateSegmentProperties(const QVector<QPoint>& perimeterCoordinates)
{
    // Calculate the area of the segment (in % of the total image area)
    int imageArea = segmentedImage.width() * segmentedImage.height();
    float segmentArea = (static_cast<float>(selectedSegment.size()) / imageArea) * 100.0f;

    // Create a string representation of the perimeter coordinates
    QString perimeterString;
    for (const QPoint& point : perimeterCoordinates)
    {
        perimeterString.append(QString("X=%1, Y=%2\n").arg(point.x()).arg(point.y()));
    }

    // Display the calculated area and perimeter coordinates
    ui->pDark_3->setText(QString::number(segmentArea));
    ui->pDark_4->setText(perimeterString);
}
