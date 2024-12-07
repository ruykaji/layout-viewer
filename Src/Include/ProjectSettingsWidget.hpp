#ifndef __PROJECT_SETTINGS_WIDGET_HPP__
#define __PROJECT_SETTINGS_WIDGET_HPP__

#include <QAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>

#include <filesystem>

namespace gui
{

struct ProjectSettings
{
  std::string m_name;
  std::string m_pdk_folder;
  std::string m_def_file;
  std::string m_guide_file;

  void
  save_to(const std::filesystem::path& path) const;

  void
  read_from(const std::filesystem::path& path);
};

class ProjectSettingsWidget : public QDialog
{
  Q_OBJECT

public:
  /** =============================== CONSTRUCTORS ================================= */

  ProjectSettingsWidget(QWidget* parent = nullptr);

public:
  /** =============================== PUBLIC METHODS =============================== */

  ProjectSettings
  get_settings() const;

private:
  /** =============================== PRIVATE METHODS ============================== */

  QHBoxLayout*
  make_file_input(const QString& label_text, const int32_t label_width, QLineEdit*& line_edit, std::function<void()> callback);

  QHBoxLayout*
  make_string_input(const QString& label_text, const int32_t label_width, QLineEdit*& line_edit, std::function<void()> callback);

  int32_t
  get_max_label_width(const QStringList& labels);

private:
  QLineEdit* m_name_edit;
  QLineEdit* m_pdk_folder_edit;
  QLineEdit* m_def_file_edit;
  QLineEdit* m_guide_file_edit;
};

}

#endif