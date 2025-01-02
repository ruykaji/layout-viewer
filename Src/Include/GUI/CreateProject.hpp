#ifndef __CREATE_PROJECT_HPP__
#define __CREATE_PROJECT_HPP__

#include <QAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>

#include <filesystem>

#include "Include/GUI/ProjectSettings.hpp"

namespace gui::create_project
{

class Widget : public QDialog
{
  Q_OBJECT

public:
  Widget(QWidget* parent = nullptr);

public:
  ProjectSettings
  get_settings() const;

private:
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