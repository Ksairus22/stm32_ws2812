import unittest
from pathlib import Path

from firmware_generator import (
    SPEC_MIN,
    BuildSettings,
    cmake_arguments,
    dynamic_range,
    flash_command,
)


class BuildSettingsTests(unittest.TestCase):
    def test_build_arguments_include_all_led_and_effect_settings(self):
        settings = BuildSettings(
            led_count=60,
            brightness=120,
            frame_interval_ms=25,
            transition_ms=400,
            saturation=210,
            trail_length=9,
            sequence=[3, 8, 1],
            durations_ms=[5000, 8000, 3000],
        )

        self.assertEqual(
            cmake_arguments(settings),
            [
                "-DWS2812_LED_COUNT=60",
                "-DEFFECT_BRIGHTNESS=120",
                "-DEFFECT_FRAME_INTERVAL_MS=25",
                "-DEFFECT_TRANSITION_MS=400",
                "-DEFFECT_SATURATION=210",
                "-DEFFECT_TRAIL_LENGTH=9",
                "-DEFFECT_SEQUENCE=3,8,1",
                "-DEFFECT_DURATIONS_MS=5000,8000,3000",
            ],
        )

    def test_rejects_mismatched_or_invalid_sequence(self):
        with self.assertRaisesRegex(ValueError, "same length"):
            BuildSettings(sequence=[0, 1], durations_ms=[1000])
        with self.assertRaisesRegex(ValueError, "LED"):
            BuildSettings(led_count=0)
        with self.assertRaisesRegex(ValueError, "mode"):
            BuildSettings(sequence=[12], durations_ms=[1000])


class DynamicRangeTests(unittest.TestCase):
    def test_range_is_centered_on_entered_value_with_percent_margin(self):
        low, high = dynamic_range(100, SPEC_MIN["transition_ms"], 5000, margin=0.5)
        self.assertEqual((low, high), (50, 150))

    def test_range_clamps_to_hard_bounds(self):
        low, high = dynamic_range(250, SPEC_MIN["brightness"], 255, margin=0.5)
        self.assertEqual((low, high), (125, 255))

    def test_invalid_number_falls_back_to_full_range(self):
        low, high = dynamic_range(None, SPEC_MIN["saturation"], 255, margin=0.5)
        self.assertEqual((low, high), (0, 255))


class FlashCommandTests(unittest.TestCase):
    def test_flash_uses_project_script_and_image(self):
        command = flash_command("C:/proj", "C:/proj/build-8/stm32_ws2812.bin")
        self.assertEqual(
            command,
            [
                "powershell.exe",
                "-NoLogo",
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(Path("C:/proj") / "tools" / "flash.ps1"),
                "-Image",
                "C:/proj/build-8/stm32_ws2812.bin",
            ],
        )


if __name__ == "__main__":
    unittest.main()
