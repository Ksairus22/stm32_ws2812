"""Desktop generator for configurable STM32 WS2812 firmware."""

from __future__ import annotations

import json
import subprocess
import tkinter as tk
from dataclasses import dataclass, field
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Iterable

MODE_NAMES = [
    "Static", "Hue sweep", "Comet", "Rainbow", "Sparkle", "Breathing",
    "Fire", "Twinkle", "Larson scanner", "Color waves", "Theater chase", "Ocean",
]
MODE_COUNT = len(MODE_NAMES)

SPEC_MIN = {
    "led_count": 1,
    "brightness": 1,
    "frame_interval_ms": 10,
    "transition_ms": 0,
    "saturation": 0,
    "trail_length": 1,
}
SPEC_MAX = {
    "led_count": 300,
    "brightness": 255,
    "frame_interval_ms": 200,
    "transition_ms": 5000,
    "saturation": 255,
    "trail_length": 64,
}


@dataclass
class BuildSettings:
    led_count: int = 8
    brightness: int = 64
    frame_interval_ms: int = 40
    transition_ms: int = 250
    saturation: int = 255
    trail_length: int = 8
    sequence: list[int] = field(default_factory=lambda: list(range(MODE_COUNT)))
    durations_ms: list[int] = field(default_factory=lambda: [4000] * MODE_COUNT)

    def __post_init__(self) -> None:
        if not 1 <= self.led_count <= 300:
            raise ValueError("LED count must be from 1 to 300")
        if not 1 <= self.brightness <= 255:
            raise ValueError("brightness must be from 1 to 255")
        if not 10 <= self.frame_interval_ms <= 200:
            raise ValueError("frame interval must be from 10 to 200 ms")
        if not 0 <= self.transition_ms <= 5000:
            raise ValueError("transition must be from 0 to 5000 ms")
        if not 0 <= self.saturation <= 255:
            raise ValueError("saturation must be from 0 to 255")
        if not 1 <= self.trail_length <= 64:
            raise ValueError("trail length must be from 1 to 64")
        if not self.sequence:
            raise ValueError("at least one mode is required")
        if len(self.sequence) != len(self.durations_ms):
            raise ValueError("sequence and durations must have the same length")
        if any(not 0 <= mode < MODE_COUNT for mode in self.sequence):
            raise ValueError("invalid mode in sequence")
        if any(not 100 <= duration <= 3_600_000 for duration in self.durations_ms):
            raise ValueError("mode duration must be from 100 to 3600000 ms")


def _csv(values: Iterable[int]) -> str:
    return ",".join(str(value) for value in values)


def cmake_arguments(settings: BuildSettings) -> list[str]:
    return [
        f"-DWS2812_LED_COUNT={settings.led_count}",
        f"-DEFFECT_BRIGHTNESS={settings.brightness}",
        f"-DEFFECT_FRAME_INTERVAL_MS={settings.frame_interval_ms}",
        f"-DEFFECT_TRANSITION_MS={settings.transition_ms}",
        f"-DEFFECT_SATURATION={settings.saturation}",
        f"-DEFFECT_TRAIL_LENGTH={settings.trail_length}",
        f"-DEFFECT_SEQUENCE={_csv(settings.sequence)}",
        f"-DEFFECT_DURATIONS_MS={_csv(settings.durations_ms)}",
    ]


def project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def dynamic_range(value: int | None, low: int, high: int, margin: float = 0.5) -> tuple[int, int]:
    """Return a slider range centred on value, ±margin*value, clamped to [low, high]."""
    if not value or value <= 0 or high <= low:
        return low, high
    span = max(1, round(abs(value) * margin))
    return max(low, value - span), min(high, value + span)


def flash_command(root: Path | str, image: Path | str) -> list[str]:
    return [
        "powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass",
        "-File", str(Path(root) / "tools" / "flash.ps1"),
        "-Image", str(image),
    ]


def load_settings(path: Path) -> BuildSettings:
    return BuildSettings(**json.loads(path.read_text(encoding="utf-8")))


def save_settings(path: Path, settings: BuildSettings) -> None:
    path.write_text(json.dumps(settings.__dict__, indent=2), encoding="utf-8")


class GeneratorApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("WS2812 Firmware Generator")
        self.geometry("900x650")
        self.settings = BuildSettings()
        self.vars = {name: tk.IntVar(value=getattr(self.settings, name)) for name in SPEC_MIN}
        self.entries = {name: tk.StringVar(value=str(getattr(self.settings, name))) for name in SPEC_MIN}
        self.displays = {name: tk.StringVar(value=str(getattr(self.settings, name))) for name in SPEC_MIN}
        self.scales: dict[str, ttk.Scale] = {}
        self.sequence: list[tuple[int, tk.IntVar]] = []
        self._build_ui()

    def _slider_entry(self, parent: tk.Widget, label: str, key: str, row: int) -> None:
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", padx=8, pady=6)
        scale = ttk.Scale(parent, from_=SPEC_MIN[key], to=SPEC_MAX[key], variable=self.vars[key], orient="horizontal", command=lambda v, k=key: self._on_slider(k, v))
        scale.grid(row=row, column=1, sticky="ew", padx=8)
        self.scales[key] = scale
        entry = ttk.Entry(parent, width=8, textvariable=self.entries[key])
        entry.grid(row=row, column=2, padx=(16, 8))
        entry.bind("<Return>", lambda _e, k=key: self._commit_entry(k))
        entry.bind("<KP_Enter>", lambda _e, k=key: self._commit_entry(k))  # numpad Enter
        entry.bind("<FocusOut>", lambda _e, k=key: self._commit_entry(k))
        ttk.Label(parent, textvariable=self.displays[key], width=7).grid(row=row, column=3, sticky="w")

    def _on_slider(self, key: str, raw: str | None = None) -> None:
        # Read the exact value from the Scale callback argument: reading back
        # through IntVar truncates (129.9 -> 129) instead of rounding.
        try:
            source = raw if raw is not None else self.vars[key].get()
            value = int(round(float(source)))
        except (ValueError, TypeError, tk.TclError):
            return
        value = max(SPEC_MIN[key], min(SPEC_MAX[key], value))
        self.vars[key].set(float(value))
        self.entries[key].set(str(value))
        self.displays[key].set(str(value))

    def _commit_entry(self, key: str) -> None:
        try:
            value = int(self.entries[key].get())
        except (ValueError, TypeError):
            self.entries[key].set(str(self.vars[key].get()))
            return
        if value < SPEC_MIN[key] or value > SPEC_MAX[key]:
            value = max(SPEC_MIN[key], min(SPEC_MAX[key], value))
            self.entries[key].set(str(value))  # reflect the clamp in the field
        low, high = dynamic_range(value, SPEC_MIN[key], SPEC_MAX[key])
        self.scales[key].configure(from_=low, to=high)
        self.vars[key].set(value)
        self.entries[key].set(str(value))
        self.displays[key].set(str(value))

    def _build_ui(self) -> None:
        tabs = ttk.Notebook(self); tabs.pack(fill="both", expand=True, padx=12, pady=12)
        general = ttk.Frame(tabs, padding=12); order = ttk.Frame(tabs, padding=12)
        tabs.add(general, text="Параметры"); tabs.add(order, text="Порядок и длительность")
        general.columnconfigure(1, weight=1)
        self._slider_entry(general, "Количество диодов", "led_count", 0)
        self._slider_entry(general, "Яркость", "brightness", 1)
        self._slider_entry(general, "Скорость режима (кадр, мс)", "frame_interval_ms", 2)
        self._slider_entry(general, "Скорость смены режимов (мс)", "transition_ms", 3)
        self._slider_entry(general, "Насыщенность", "saturation", 4)
        self._slider_entry(general, "Длина хвоста", "trail_length", 5)
        ttk.Label(general, text="Число можно ввести вручную (Enter) — диапазон ползунка перестроится вокруг него ±50%.").grid(row=7, column=0, columnspan=4, sticky="w", pady=18)
        self.order_list = tk.Listbox(order, height=18, exportselection=False)
        self.order_list.grid(row=0, column=0, rowspan=4, sticky="nsew", padx=(0, 12)); order.columnconfigure(0, weight=1); order.rowconfigure(0, weight=1)
        buttons = ttk.Frame(order); buttons.grid(row=0, column=1, sticky="ns")
        self.mode_choice = tk.StringVar(value=MODE_NAMES[0])
        ttk.Combobox(buttons, textvariable=self.mode_choice, values=MODE_NAMES, state="readonly", width=18).pack(fill="x", pady=(0, 6))
        for text, command in (("Добавить", self._add_mode), ("Удалить", self._remove_mode), ("Вверх", self._move_up), ("Вниз", self._move_down)):
            ttk.Button(buttons, text=text, command=command).pack(fill="x", pady=4)
        ttk.Label(order, text="Длительность выбранного режима, мс (Enter — применить)").grid(row=1, column=1, sticky="w", pady=(20, 4))
        self.duration = tk.IntVar(value=4000)
        duration_spin = ttk.Spinbox(order, from_=100, to=3600000, increment=100, textvariable=self.duration, width=12)
        duration_spin.grid(row=2, column=1, sticky="w")
        duration_spin.bind("<Return>", lambda _e: self._apply_duration())
        ttk.Button(order, text="Применить длительность", command=self._apply_duration).grid(row=3, column=1, sticky="w", pady=8)
        self.order_list.bind("<<ListboxSelect>>", self._select_mode)
        for mode in self.settings.sequence: self._append_mode(mode, 4000)
        bottom = ttk.Frame(self); bottom.pack(fill="x", padx=12, pady=(0, 12))
        for text, command in (("Загрузить", self._load), ("Сохранить настройки", self._save), ("Собрать .bin", self._build), ("Прошить ST-LINK", self._flash)):
            ttk.Button(bottom, text=text, command=command).pack(side="left", padx=4)
        self.status = tk.StringVar(value="Готово"); ttk.Label(bottom, textvariable=self.status).pack(side="right")

    def _append_mode(self, mode: int, duration: int) -> None:
        self.sequence.append((mode, tk.IntVar(value=duration))); self._refresh_order()

    def _add_mode(self) -> None: self._append_mode(MODE_NAMES.index(self.mode_choice.get()), 4000)

    def _remove_mode(self) -> None:
        selected = self.order_list.curselection()
        if selected and len(self.sequence) > 1: del self.sequence[selected[0]]; self._refresh_order()

    def _move_up(self) -> None:
        selected = self.order_list.curselection()
        if selected and selected[0] > 0:
            i = selected[0]; self.sequence[i - 1], self.sequence[i] = self.sequence[i], self.sequence[i - 1]; self._refresh_order(i - 1)

    def _move_down(self) -> None:
        selected = self.order_list.curselection()
        if selected and selected[0] < len(self.sequence) - 1:
            i = selected[0]; self.sequence[i + 1], self.sequence[i] = self.sequence[i], self.sequence[i + 1]; self._refresh_order(i + 1)

    def _select_mode(self, _event: object) -> None:
        selected = self.order_list.curselection()
        if selected: self.duration.set(self.sequence[selected[0]][1].get())

    def _apply_duration(self) -> None:
        selected = self.order_list.curselection()
        if not selected:
            messagebox.showinfo("Длительность", "Выберите строку режима в списке.")
            return
        try:
            value = int(self.duration.get())
        except ValueError:
            messagebox.showerror("Длительность", "Введите целое число миллисекунд (100–3600000).")
            return
        index = selected[0]
        self.sequence[index] = (self.sequence[index][0], tk.IntVar(value=value))
        self._refresh_order(index)

    def _refresh_order(self, select: int | None = None) -> None:
        self.order_list.delete(0, "end")
        for i, (mode, duration) in enumerate(self.sequence): self.order_list.insert("end", f"{i + 1}. {MODE_NAMES[mode]} — {duration.get()} мс")
        if select is not None: self.order_list.selection_set(select)

    def _read_settings(self) -> BuildSettings:
        selected = self.order_list.curselection()
        if selected: self.sequence[selected[0]][1].set(self.duration.get()); self._refresh_order(selected[0])
        return BuildSettings(**{key: value.get() for key, value in self.vars.items()}, sequence=[mode for mode, _ in self.sequence], durations_ms=[value.get() for _, value in self.sequence])

    def _save(self) -> None:
        try: settings = self._read_settings()
        except ValueError as error: messagebox.showerror("Параметры", str(error)); return
        path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON", "*.json")])
        if path: save_settings(Path(path), settings); self.status.set("Настройки сохранены")

    def _load(self) -> None:
        path = filedialog.askopenfilename(filetypes=[("JSON", "*.json")])
        if not path: return
        try: settings = load_settings(Path(path))
        except (OSError, ValueError, json.JSONDecodeError) as error: messagebox.showerror("Загрузка", str(error)); return
        for key, value in self.vars.items(): value.set(getattr(settings, key))
        self.sequence = []
        for mode, duration in zip(settings.sequence, settings.durations_ms): self.sequence.append((mode, tk.IntVar(value=duration)))
        self._refresh_order(); self.status.set("Настройки загружены")
        for key in SPEC_MIN:
            self.entries[key].set(str(self.vars[key].get()))
            self.displays[key].set(str(self.vars[key].get()))
            self._commit_entry(key)

    def _build(self) -> None:
        try: settings = self._read_settings()
        except ValueError as error: messagebox.showerror("Параметры", str(error)); return
        build_dir = project_root() / f"build-generated-{settings.led_count}"
        self.status.set("Сборка..."); self.update_idletasks()
        try:
            subprocess.run(["cmake", "-S", str(project_root()), "-B", str(build_dir), "-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={project_root() / 'cmake' / 'arm-gcc.cmake'}", *cmake_arguments(settings)], check=True, cwd=project_root())
            subprocess.run(["cmake", "--build", str(build_dir)], check=True, cwd=project_root())
        except (OSError, subprocess.CalledProcessError) as error:
            self.status.set("Ошибка сборки"); messagebox.showerror("Сборка", str(error)); return
        output = build_dir / "stm32_ws2812.bin"
        self.status.set(f"Готово: {output}"); messagebox.showinfo("Готово", f"Файл создан:\n{output}")

    def _flash(self) -> None:
        candidates = sorted(project_root().glob("build-generated-*/stm32_ws2812.bin"), key=lambda p: p.stat().st_mtime, reverse=True)
        if not candidates:
            messagebox.showerror("Прошивка", "Сначала соберите .bin (кнопка «Собрать .bin»).")
            return
        image = candidates[0]
        if not messagebox.askyesno("Прошивка", f"Прошить через ST-LINK файл:\n{image}?"):
            return
        self.status.set("Прошивка...")
        try:
            result = subprocess.run(flash_command(project_root(), image), cwd=project_root(), capture_output=True, text=True, timeout=120)
        except (OSError, subprocess.TimeoutExpired) as error:
            self.status.set("Ошибка прошивки"); messagebox.showerror("Прошивка", str(error)); return
        if result.returncode != 0:
            self.status.set("Ошибка прошивки")
            messagebox.showerror("Прошивка", result.stderr or result.stdout or "OpenOCD вернул ошибку.")
            return
        self.status.set(f"Прошито: {image}")


if __name__ == "__main__":
    GeneratorApp().mainloop()
