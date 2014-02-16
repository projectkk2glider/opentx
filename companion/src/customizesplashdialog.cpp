#include "customizesplashdialog.h"
#include "ui_customizesplashdialog.h"

#include <QtGui>
#include "helpers.h"
#include "burndialog.h"
#include "splashlibrary.h"
#include "flashinterface.h"

//*** Side Class ***

Side::Side(){
  imageLabel = 0;
  fileNameEdit = 0;
  saveButton = 0;
  saveToFileName = new QString("");
  source = new Source(UNDEFINED);
  format = new LCDFormat(LCDTARANIS);
}

void Side::copyImage( Side side )
{
 if ((*source!=UNDEFINED) && (*side.source!=UNDEFINED))
    imageLabel->setPixmap(*side.imageLabel->pixmap());
}

bool Side::displayImage( QString fileName, Source pictSource )
{
  QImage image;

  if (fileName.isEmpty()) {
    return false;
  }
  if (pictSource == FW ){
    FlashInterface flash(fileName);
    if (!flash.hasSplash())
      return false;
    else
      image = flash.getSplash();
      *format = (flash.getSplashWidth()==WIDTH_TARANIS ? LCDTARANIS : LCD9X);
  }
  else {
   image.load(fileName);
   if (pictSource== PICT)
     *format = image.width()>WIDTH_9X ? LCDTARANIS : LCD9X;
  }
  if (image.isNull()) {
    return false;
  }
  if (*format==LCDTARANIS) {
    image=image.convertToFormat(QImage::Format_RGB32);
    QRgb col;
    int gray;
    int width = image.width();
    int height = image.height();
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            col = image.pixel(i, j);
            gray = qGray(col);
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }      
    imageLabel->setPixmap(QPixmap::fromImage(image.scaled(imageLabel->width()/2, imageLabel->height()/2)));
  }
  else
    imageLabel->setPixmap(QPixmap::fromImage(image.scaled(imageLabel->width()/2, imageLabel->height()/2).convertToFormat(QImage::Format_Mono)));
  
  if (*format == LCD9X)
      imageLabel->setFixedSize(WIDTH_9X*2, HEIGHT_9X*2);
  else
      imageLabel->setFixedSize(WIDTH_TARANIS*2, HEIGHT_TARANIS*2);
  
  switch (pictSource){
    case FW:
         fileNameEdit->setText(QObject::tr("FW: %1").arg(fileName));
         *saveToFileName = fileName;
         *source=FW;
       break;
    case PICT:
         fileNameEdit->setText(QObject::tr("Pict: %1").arg(fileName));
         *saveToFileName = fileName;
         *source=PICT;
       break;
    case PROFILE:
         fileNameEdit->setText(QObject::tr("Profile image"));
         *saveToFileName = fileName;
         *source=PROFILE;
      break;
    default:
     break;
  }
  saveButton->setEnabled(true);
  libraryButton->setEnabled(true);
  invertButton->setEnabled(true);
  return true;
}

bool Side::refreshImage()
{
  return displayImage( *saveToFileName, *source );
}

bool Side::saveImage()
{
  QSettings settings;

  if (*source == FW )
  {
    FlashInterface flash(*saveToFileName);
    if (!flash.hasSplash()) {
      return false;
    }
    QImage image = imageLabel->pixmap()->toImage().scaled(flash.getSplashWidth(), flash.getSplashHeight());
    if (flash.setSplash(image) && (flash.saveFlash(*saveToFileName) > 0)) {
      settings.setValue("lastFlashDir", QFileInfo(*saveToFileName).dir().absolutePath());
    }
    else {
      return false;
    }
  }
  else if (*source == PICT) {
    QImage image = imageLabel->pixmap()->toImage().scaled(imageLabel->width()/2, imageLabel->height()/2).convertToFormat(QImage::Format_Indexed8);
    if (image.save(*saveToFileName)) {
      settings.setValue("lastImagesDir", QFileInfo(*saveToFileName).dir().absolutePath());
    }
    else {
      return false;
    }
  }
  else if (*source == PROFILE) {
    QImage image = imageLabel->pixmap()->toImage().scaled(imageLabel->width()/2, imageLabel->height()/2).convertToFormat(QImage::Format_Indexed8);
    if (!image.save(*saveToFileName)) {
      return false;
    }
  }
  return true;
}

//*** customizeSplashDialog Class ***

customizeSplashDialog::customizeSplashDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::customizeSplashDialog)
{
  ui->setupUi(this);
  ui->leftLibraryButton->setIcon(CompanionIcon("library.png"));
  ui->rightLibraryButton->setIcon(CompanionIcon("library.png"));
  
  left.imageLabel =  ui->leftImageLabel;
  right.imageLabel = ui->rightImageLabel;
  left.fileNameEdit =  ui->leftFileNameEdit;
  right.fileNameEdit = ui->rightFileNameEdit;
  left.saveButton =  ui->leftSaveButton;
  right.saveButton =  ui->rightSaveButton;
  left.libraryButton = ui->leftLibraryButton;
  right.libraryButton = ui->rightLibraryButton;
  left.invertButton = ui->leftInvertButton;
  right.invertButton = ui->rightInvertButton;

  loadProfile(left);
  resize(0,0);
}

customizeSplashDialog::~customizeSplashDialog()
{
  delete ui;
}

void customizeSplashDialog::on_copyRightToLeftButton_clicked() {
  left.copyImage(right);
}
void customizeSplashDialog::on_copyLeftToRightButton_clicked() {
right.copyImage(left);
}


void customizeSplashDialog::on_leftLoadFwButton_clicked() {loadFirmware(left);}
void customizeSplashDialog::on_rightLoadFwButton_clicked() {loadFirmware(right);}
void customizeSplashDialog::loadFirmware(Side side)
{
  QSettings settings;
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), settings.value("lastFlashDir").toString(), FLASH_FILES_FILTER);
  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, FW ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load embedded FW image from %1.").arg(fileName));
    else
    settings.setValue("lastFlashDir", QFileInfo(fileName).dir().absolutePath());
  }
}

void customizeSplashDialog::on_leftLoadPictButton_clicked() {loadPicture(left);}
void customizeSplashDialog::on_rightLoadPictButton_clicked() {loadPicture(right);}
void customizeSplashDialog::loadPicture(Side side)
{
  QString supportedImageFormats;
  for (int formatIndex = 0; formatIndex < QImageReader::supportedImageFormats().count(); formatIndex++) {
    supportedImageFormats += QLatin1String(" *.") + QImageReader::supportedImageFormats()[formatIndex];
  }
  QSettings settings;
  QString fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image to load"), settings.value("lastImagesDir").toString(), tr("Images (%1)").arg(supportedImageFormats));

  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, PICT ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load the image file %1.").arg(fileName));
    else
      settings.setValue("lastImagesDir", QFileInfo(fileName).dir().absolutePath());
  }
}

void customizeSplashDialog::on_leftLoadProfileButton_clicked() {loadProfile(left);}
void customizeSplashDialog::on_rightLoadProfileButton_clicked() {loadProfile(right);}
void customizeSplashDialog::loadProfile(Side side)
{
  QSettings settings;
  QString fileName=settings.value("SplashFileName","").toString();

  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, PROFILE ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load the profile image %1.").arg(fileName));
  }
}

void customizeSplashDialog::on_leftLibraryButton_clicked(){libraryButton_clicked(left);}
void customizeSplashDialog::on_rightLibraryButton_clicked(){libraryButton_clicked(right);}
void customizeSplashDialog::libraryButton_clicked( Side side )
{
  QString fileName;
  splashLibrary *ld = new splashLibrary(this,&fileName);
  ld->exec();
  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, UNDEFINED ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load the library image %1.").arg(fileName));
  }
}

void customizeSplashDialog::on_leftSaveButton_clicked(){saveButton_clicked(left);}
void customizeSplashDialog::on_rightSaveButton_clicked(){saveButton_clicked(right);}
void customizeSplashDialog::saveButton_clicked( Side side )
{
  if (side.saveImage()){
    QMessageBox::information(this, tr("File Saved"), tr("The image was saved to the file %1").arg(*side.saveToFileName));
    if ( !side.refreshImage()){
      QMessageBox::critical(this, tr("Image Refresh Error"), tr("Failed to refresh image from file %1").arg(*side.saveToFileName));
    }
  }
  else
    QMessageBox::critical(this, tr("File Save Error"), tr("Failed to write image to %1").arg(*side.saveToFileName));
}

void customizeSplashDialog::on_leftInvertButton_clicked(){invertButton_clicked(left);}
void customizeSplashDialog::on_rightInvertButton_clicked(){invertButton_clicked(right);}
void customizeSplashDialog::invertButton_clicked(Side side)
{
  QImage image = side.imageLabel->pixmap()->toImage();
  image.invertPixels();
  side.imageLabel->setPixmap(QPixmap::fromImage(image));
}


