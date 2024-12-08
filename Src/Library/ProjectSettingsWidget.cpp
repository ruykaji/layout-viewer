#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <fstream>

#include "Include/ProjectSettingsWidget.hpp"

namespace gui::project_settings
{

void
ProjectSettings::save_to(const std::filesystem::path& path) const
{
  if(std::filesystem::exists(path))
    {
      throw std::runtime_error("An exception has occurred while trying to save project settings.");
    }

  std::ofstream out_file(path);
  std::size_t   length;

  length = m_name.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_name.data(), length);

  length = m_pdk_folder.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_pdk_folder.data(), length);

  length = m_def_file.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_def_file.data(), length);

  length = m_guide_file.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_guide_file.data(), length);

  out_file.close();
}

void
ProjectSettings::read_from(const std::filesystem::path& path)
{
  if(!std::filesystem::exists(path))
    {
      throw std::runtime_error("An exception has occurred while trying to load project settings.");
    }

  std::ifstream in_file(path);
  std::size_t   length;

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_name.resize(length);
  in_file.read(&m_name[0], length);

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_pdk_folder.resize(length);
  in_file.read(&m_pdk_folder[0], length);

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_def_file.resize(length);
  in_file.read(&m_def_file[0], length);
  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));

  m_guide_file.resize(length);
  in_file.read(&m_guide_file[0], length);
}

/** =============================== CONSTRUCTORS ================================= */

Widget::Widget(QWidget* parent)
    : QDialog(parent)
{
  setWindowTitle("Project Settings");

  const int32_t max_label_width = get_max_label_width({ "PDK Folder:", "DEF File:", "Design File:" });

  auto          name_layout     = make_string_input("Project Name:", max_label_width, m_name_edit, [this]() {
    bool    ok;
    QString name = QInputDialog::getText(this, "", "Project Name", QLineEdit::Normal, "", &ok);

    if(!name.isEmpty())
      {
        m_name_edit->setText(name);
      }
  });

  auto          pdk_layout      = make_file_input("PDK Folder:", max_label_width, m_pdk_folder_edit, [this]() {
    QString file_folder = QFileDialog::getExistingDirectory(this, "Select PDK Folder", "");

    if(!file_folder.isEmpty())
      {
        m_pdk_folder_edit->setText(file_folder);
      }
  });

  auto          def_layout      = make_file_input("DEF File:", max_label_width, m_def_file_edit, [this]() {
    QString file_name = QFileDialog::getOpenFileName(this, "Select DEF File", "", "DEF Files (*.def)");

    if(!file_name.isEmpty())
      {
        m_def_file_edit->setText(file_name);
      }
  });

  auto          guide_layout    = make_file_input("Guide File:", max_label_width, m_guide_file_edit, [this]() {
    QString file_name = QFileDialog::getOpenFileName(this, "Select Guide File", "", "Guide Files (*.guide)");

    if(!file_name.isEmpty())
      {
        m_guide_file_edit->setText(file_name);
      }
  });

  QVBoxLayout*  layout          = new QVBoxLayout(this);
  layout->addLayout(name_layout);
  layout->addLayout(pdk_layout);
  layout->addLayout(def_layout);
  layout->addLayout(guide_layout);

  /** Create and Cancel buttons */
  QHBoxLayout* button_layout = new QHBoxLayout();

  QPushButton* create_button = new QPushButton("Create");
  connect(create_button, &QPushButton::clicked, this, &QDialog::accept);

  QPushButton* cancel_button = new QPushButton("Cancel");
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  button_layout->addWidget(create_button);
  button_layout->addWidget(cancel_button);
  layout->addLayout(button_layout);
}

/** =============================== PUBLIC METHODS =============================== */

ProjectSettings
Widget::get_settings() const
{
  ProjectSettings settings;
  settings.m_name       = m_name_edit->text().toStdString();
  settings.m_pdk_folder = m_pdk_folder_edit->text().toStdString();
  settings.m_def_file   = m_def_file_edit->text().toStdString();
  settings.m_guide_file = m_guide_file_edit->text().toStdString();

  return settings;
}

/** =============================== PRIVATE METHODS ============================== */

QHBoxLayout*
Widget::make_file_input(const QString& label_text, const int32_t label_width, QLineEdit*& line_edit, std::function<void()> callback)
{
  QLabel* label = new QLabel(label_text);
  label->setFixedWidth(label_width);

  line_edit = new QLineEdit();
  line_edit->setFixedWidth(500);

  QPushButton* browse_button = new QPushButton("Browse...");
  QObject::connect(browse_button, &QPushButton::clicked, callback);

  QHBoxLayout* layout = new QHBoxLayout();
  layout->addWidget(label);
  layout->addWidget(line_edit);
  layout->addWidget(browse_button);

  return layout;
}

QHBoxLayout*
Widget::make_string_input(const QString& label_text, const int32_t label_width, QLineEdit*& line_edit, std::function<void()> callback)
{
  QLabel* label = new QLabel(label_text);
  label->setFixedWidth(label_width);

  line_edit = new QLineEdit();
  line_edit->setFixedWidth(500);

  QHBoxLayout* layout = new QHBoxLayout();
  layout->setAlignment(Qt::AlignLeft);
  layout->addWidget(label);
  layout->addWidget(line_edit);

  return layout;
}

int32_t
Widget::get_max_label_width(const QStringList& labels)
{
  int32_t            maxWidth = 0;
  const QFontMetrics metrics(font());

  for(const QString& label : labels)
    {
      maxWidth = qMax(maxWidth, metrics.horizontalAdvance(label));
    }

  return maxWidth + 10;
}

}