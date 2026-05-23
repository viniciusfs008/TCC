#!/bin/bash

set -e  # para parar se der erro

# ==============================

# CONFIGURAÇÕES

# ==============================

IMG_NAME="minix.img"
IMG_SIZE_MB=50
SRC_DIR="$1"           # diretório passado como argumento
MOUNT_POINT="/mnt/minix_tmp"

# ==============================

# VALIDAÇÕES

# ==============================

if [ -z "$SRC_DIR" ]; then
echo "Uso: $0 <diretorio_origem>"
exit 1
fi

if [ ! -d "$SRC_DIR" ]; then
echo "Erro: diretório '$SRC_DIR' não existe"
exit 1
fi

# ==============================

# CRIAR IMAGEM

# ==============================

echo "[1/5] Criando imagem de $IMG_SIZE_MB MB..."
dd if=/dev/zero of="$IMG_NAME" bs=1M count=$IMG_SIZE_MB status=progress

# ==============================

# FORMATAR MINIX 3

# ==============================

echo "[2/5] Formatando como Minix 3..."
mkfs.minix -3 "$IMG_NAME"

# ==============================

# MONTAR

# ==============================

echo "[3/5] Montando imagem..."
sudo mkdir -p "$MOUNT_POINT"
sudo mount -o loop -t minix "$IMG_NAME" "$MOUNT_POINT"

# ==============================

# COPIAR ARQUIVOS

# ==============================

echo "[4/5] Copiando arquivos de $SRC_DIR..."
sudo cp -r "$SRC_DIR"/* "$MOUNT_POINT"/

# ==============================

# DESMONTAR

# ==============================

echo "[5/5] Desmontando imagem..."
sudo umount "$MOUNT_POINT"
sudo rmdir "$MOUNT_POINT"

echo "✅ Imagem criada com sucesso: $IMG_NAME"
