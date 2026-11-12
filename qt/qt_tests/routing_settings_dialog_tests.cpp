#include "testing/testing.hpp"

#include "qt/routing_settings_dialog.hpp"

#include "map/framework.hpp"

#include "routing/router.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#include <map>
#include <optional>
#include <string>

namespace routing_settings_dialog_tests
{
using namespace routing;

class RoutingSettingsFixture
{
public:
  RoutingSettingsFixture()
  {
    static int argc = 0;
    static QApplication app(argc, nullptr);
    for (auto const key : {"show_turns_desktop", "use_debug_guide_desktop", "use_cached_settings_desktop",
                           "start_coords_desktop", "finish_coords_desktop", "router_type_desktop"})
    {
      std::string value;
      if (settings::Get(key, value))
        m_saved[key] = value;
      else
        m_saved[key] = std::nullopt;
      settings::Delete(key);
    }
  }

  ~RoutingSettingsFixture()
  {
    for (auto const & [key, value] : m_saved)
    {
      if (value)
        settings::Set(key, *value);
      else
        settings::Delete(key);
    }
  }

  static QCheckBox * Option(qt::RoutingSettings & dialog, char const * name)
  {
    auto * checkbox = dialog.findChild<QCheckBox *>(name);
    CHECK(checkbox, (name));
    return checkbox;
  }

  static void Accept(qt::RoutingSettings & dialog)
  {
    auto const inputs = dialog.findChildren<QLineEdit *>();
    CHECK_EQUAL(inputs.size(), 2, ());
    inputs[0]->setText("1, 1");
    inputs[1]->setText("1, 2");
    QTimer::singleShot(0, &dialog, &QDialog::accept);
    dialog.ShowModal();
  }

private:
  std::map<std::string, std::optional<std::string>> m_saved;
  RoutingOptionSetter m_car{0, VehicleType::Car};
  RoutingOptionSetter m_bicycle{0, VehicleType::Bicycle};
  RoutingOptionSetter m_pedestrian{0, VehicleType::Pedestrian};
};

UNIT_CLASS_TEST(RoutingSettingsFixture, RoutingSettings_ModeChangesAndCancelKeepProfilesSeparate)
{
  Framework framework(FrameworkParams{}, false);
  qt::RoutingSettings dialog(nullptr, framework, kml::kInvalidTrackId);
  auto * mode = dialog.findChild<QComboBox *>();
  auto * tramline = Option(dialog, "avoid_tramlines");
  auto * ferry = Option(dialog, "avoid_ferry");
  auto * toll = Option(dialog, "avoid_tolls");

  TEST(tramline->isHidden(), ());
  TEST(!toll->isHidden(), ());
  toll->setChecked(true);
  mode->setCurrentIndex(static_cast<int>(RouterType::Bicycle));
  TEST(!tramline->isHidden(), ());
  TEST(toll->isHidden(), ());
  tramline->setChecked(true);
  ferry->setChecked(true);

  mode->setCurrentIndex(static_cast<int>(RouterType::Pedestrian));
  TEST(tramline->isHidden(), ());
  TEST(!ferry->isHidden(), ());
  TEST(!ferry->isChecked(), ());
  for (auto const router : {RouterType::Transit, RouterType::Ruler})
  {
    mode->setCurrentIndex(static_cast<int>(router));
    for (auto const name : {"avoid_tolls", "avoid_unpaved", "avoid_ferry", "avoid_motorways", "avoid_tramlines"})
      TEST(Option(dialog, name)->isHidden(), (name, router));
  }
  mode->setCurrentIndex(static_cast<int>(RouterType::Bicycle));
  TEST(tramline->isChecked(), ());
  TEST(ferry->isChecked(), ());
  mode->setCurrentIndex(static_cast<int>(RouterType::Vehicle));
  TEST(toll->isChecked(), ());
  TEST(!ferry->isChecked(), ());

  QTimer::singleShot(0, &dialog, &QDialog::reject);
  dialog.ShowModal();
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
    TEST_EQUAL(RoutingOptions::LoadFromSettings(vehicle).GetOptions(), 0, (vehicle));
}

UNIT_CLASS_TEST(RoutingSettingsFixture, RoutingSettings_SaveAndTogglePreserveOtherOptions)
{
  Framework framework(FrameworkParams{}, false);
  {
    qt::RoutingSettings dialog(nullptr, framework, kml::kInvalidTrackId);
    Option(dialog, "avoid_tolls")->setChecked(true);
    Option(dialog, "avoid_unpaved")->setChecked(true);
    dialog.findChild<QComboBox *>()->setCurrentIndex(static_cast<int>(RouterType::Bicycle));
    Option(dialog, "avoid_tramlines")->setChecked(true);
    Option(dialog, "avoid_ferry")->setChecked(true);
    Accept(dialog);
  }
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Car).GetOptions(),
             RoutingOptions::Toll | RoutingOptions::Dirty, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).GetOptions(),
             RoutingOptions::Ferry | RoutingOptions::Tramline, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Pedestrian).GetOptions(), 0, ());
  {
    qt::RoutingSettings dialog(nullptr, framework, kml::kInvalidTrackId);
    TEST(Option(dialog, "avoid_tramlines")->isChecked(), ());
    Option(dialog, "avoid_tramlines")->setChecked(false);
    Accept(dialog);
  }
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).GetOptions(), RoutingOptions::Ferry, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Car).GetOptions(),
             RoutingOptions::Toll | RoutingOptions::Dirty, ());
}
}  // namespace routing_settings_dialog_tests
