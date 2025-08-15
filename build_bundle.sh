#!/bin/bash
flatpak-builder --install --user --force-clean flatpak-app org.speckitor.NekoDownloader.json
flatpak build-bundle ~/.local/share/flatpak/repo NekoDownloader.flatpak org.speckitor.NekoDownloader
