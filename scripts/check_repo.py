#!/usr/bin/env python3
"""Validate public-repository contracts that require no physical hardware."""
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REQUIRED = [
    '.gitattributes', '.github/workflows/validate.yml', '.gitignore', '.markdownlint-cli2.jsonc',
    'HARDWARE.md', 'LICENSE', 'README.md', 'SECURITY.md', 'THIRD_PARTY_NOTICES.md',
    'docs/GITHUB_METADATA.md', 'docs/HARDWARE_LAB_CARD.md', 'docs/PROJECT_STATUS.md',
    'docs/PROTOCOL.md', 'docs/SOURCE_PROVENANCE.md', 'docs/VERIFICATION.md',
    'firmware/include/webpage.h', 'firmware/platformio.ini', 'firmware/src/config.h',
    'firmware/src/wifi_credentials.example.h',
    'firmware/src/main.cpp', 'hardware/BOM.csv', 'hardware/wiring-diagram.svg',
    'scripts/check_repo.py', 'scripts/secret_scan.py', 'scripts/verify.sh',
    'tests/test_source_contracts.py',
]
FORBIDDEN_NAMES = {'.env', 'id_rsa', 'id_ed25519', 'local.properties', 'credentials.h', 'wifi_credentials.h'}
FORBIDDEN_DIRS = {'.git', '.pio', '.vscode', '.idea', '.vs', 'build', 'dist', '__pycache__'}
FORBIDDEN_SUFFIXES = {'.o', '.a', '.elf', '.bin', '.map', '.hex', '.pyc', '.apk', '.aab', '.pem', '.key', '.zip', '.7z', '.tar', '.gz'}


def files(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        raw = b''
    if raw:
        return [root / item.decode('utf-8', 'surrogateescape') for item in raw.split(b'\0') if item]
    return sorted(
        path for path in root.rglob('*')
        if path.is_file() and not any(part in FORBIDDEN_DIRS for part in path.relative_to(root).parts)
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    root = Path(parser.parse_args().root).resolve()
    errors: list[str] = []
    for rel in REQUIRED:
        if not (root / rel).is_file():
            errors.append(f'missing required file: {rel}')
    checked = files(root)
    for path in checked:
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'forbidden local/config file: {rel}')
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f'forbidden generated directory: {rel}')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'forbidden binary/archive/key artifact: {rel}')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'file exceeds 5 MiB: {rel}')

    contracts = {
        'firmware/platformio.ini': [
            'platform = espressif32@6.13.0', 'ESP32Async/ESPAsyncWebServer@3.6.0',
            'ESP32Async/AsyncTCP@3.4.10',
            'claws/BH1750@1.3.0', 'bblanchon/ArduinoJson@6.21.6',
        ],
        'firmware/src/config.h': [
            'LED_PINS[] = {25, 26, 23, 19}', 'I2C_SDA_PIN = 21', 'I2C_SCL_PIN = 22',
            'MAX_REQUEST_BODY_BYTES = 512',
        ],
        'firmware/src/main.cpp': [
            '#include <WiFi.h>', 'void handleLedBody(', 'void handleConfigBody(', 'malloc(total + 1)',
            'deserializeJson(document, buffer, total)', 'WiFi.mode(WIFI_STA)', 'WiFi.mode(WIFI_OFF)',
            'sensorAvailable = false', 'document["sensor"] = "unavailable"', 'document["lux"] = nullptr',
            'automatic output changes are disabled', 'digitalWrite(pin, LOW)',
            'server.onNotFound', '"not_found"', 'unsupported_countdown',
        ],
        'firmware/include/webpage.h': [
            '仅限可信局域网与低压 LED 负载', "data.sensor!=='available'", 'setInterval(refresh,3000)',
        ],
        'firmware/src/wifi_credentials.example.h': [
            '#define WIFI_SSID ""', '#define WIFI_PASSWORD ""', 'never starts Wi-Fi or',
        ],
        'README.md': [ '没有 TLS、身份认证、权限模型', '市电、继电器、公共/应急照明',
        ],
        'docs/SOURCE_PROVENANCE.md': [
            'smart_light_controller', '1e25da4a99550f435736eee41fdaf0b4fcd6f596adbbd4f8d38d0b1221824a97',
            'AsyncJson', 'Access-Control-Allow-Origin: *', 'wifi_credentials.h',
        ],
    }
    for rel, values in contracts.items():
        path = root / rel
        if not path.is_file():
            continue
        text = path.read_text(encoding='utf-8')
        for value in values:
            if value not in text:
                errors.append(f'fact contract missing in {rel}: {value}')

    main_cpp = (root / 'firmware/src/main.cpp').read_text(encoding='utf-8') if (root / 'firmware/src/main.cpp').is_file() else ''
    webpage = (root / 'firmware/include/webpage.h').read_text(encoding='utf-8') if (root / 'firmware/include/webpage.h').is_file() else ''
    for forbidden in ['Access-Control-Allow-Origin', 'HTTP_OPTIONS', '"/api/reset"', 'resetSettings()', 'fonts.googleapis.com', 'cdnjs.cloudflare.com']:
        if forbidden in main_cpp or forbidden in webpage:
            errors.append(f'public firmware contains forbidden network/reset behavior: {forbidden}')
    for forbidden in ['WiFiManager', 'AsyncCallbackJsonWebHandler', '#include <AsyncJson.h>']:
        if forbidden in main_cpp:
            errors.append(f'public firmware contains forbidden dependency behavior: {forbidden}')
    if 'preferences.getBool("autoMode"' in main_cpp or '"enabled"' in main_cpp[main_cpp.find('void loadConfig()'):main_cpp.find('void saveConfig()')]:
        errors.append('startup must not restore an output or automatic-mode state')
    for rel in ['README.md', 'docs/PROJECT_STATUS.md', 'docs/HARDWARE_LAB_CARD.md']:
        path = root / rel
        text = path.read_text(encoding='utf-8').lower() if path.is_file() else ''
        for claim in ['system online', 'current hardware verified', 'production ready']:
            if claim in text:
                errors.append(f'unsupported claim in {rel}: {claim}')
    try:
        ET.parse(root / 'hardware/wiring-diagram.svg')
    except (ET.ParseError, OSError) as exc:
        errors.append(f'invalid wiring SVG: {exc}')
    try:
        rows = list(csv.DictReader((root / 'hardware/BOM.csv').open(newline='', encoding='utf-8')))
        if len(rows) < 8:
            errors.append('BOM must contain at least 8 component rows')
    except (OSError, csv.Error) as exc:
        errors.append(f'invalid BOM.csv: {exc}')
    if errors:
        print('Repository check: FAIL', file=sys.stderr)
        for item in sorted(set(errors)):
            print(f'- {item}', file=sys.stderr)
        return 1
    print(f'Repository check: PASS ({len(checked)} files checked)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
