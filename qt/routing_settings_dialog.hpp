#pragma once

#include "kml/type_utils.hpp"

#include "routing/routing_options.hpp"

#include "geometry/latlon.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Framework;

namespace qt
{
class RoutingSettings : public QDialog
{
public:
  static void LoadSession(Framework & framework);

  static bool TurnsEnabled();
  static bool UseDebugGuideTrack();
  static std::optional<ms::LatLon> GetCoords(bool start);

  RoutingSettings(QWidget * parent, Framework & framework, kml::TrackId guideTrack);
  void ShowModal();

private:
  static std::optional<ms::LatLon> GetCoordsFromString(std::string const & input);

  void ShowMessage(QString const & message);
  bool ValidateAndSaveCoordsFromInput();
  bool SaveSettings();
  void LoadSettings();
  void UpdateAvoidanceOptions();

  Framework & m_framework;

  QLineEdit * m_startInput;
  QLineEdit * m_finishInput;

  QComboBox * m_routerType;
  std::array<routing::RoutingOptions, static_cast<size_t>(routing::VehicleType::Count)> m_avoidOptions;
  std::vector<std::pair<routing::RoutingOptions::Road, QCheckBox *>> m_avoidCheckboxes;
  QCheckBox * m_showTurnsCheckbox;
  QCheckBox * m_useDebugGuideCheckbox;
  QCheckBox * m_saveSessionCheckbox;
};
}  // namespace qt
