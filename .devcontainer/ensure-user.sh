#!/usr/bin/env bash
set -euo pipefail

NEW_USER="${1:-earthling}"
DESIRED_UID="${2:-1000}"
DESIRED_GID="${3:-1000}"

if getent passwd "${DESIRED_UID}" >/dev/null; then
  EXISTING_USER="$(getent passwd "${DESIRED_UID}" | cut -d: -f1)"
  if [ "${EXISTING_USER}" != "${NEW_USER}" ]; then
    EXISTING_GID="$(id -g "${EXISTING_USER}")"
    EXISTING_GROUP="$(getent group "${EXISTING_GID}" | cut -d: -f1)"
    if [ "${EXISTING_GROUP}" != "${NEW_USER}" ]; then
      groupmod -n "${NEW_USER}" "${EXISTING_GROUP}"
    fi
    usermod -l "${NEW_USER}" -d "/home/${NEW_USER}" -m "${EXISTING_USER}"
  fi
else
  getent group "${DESIRED_GID}" >/dev/null || groupadd -g "${DESIRED_GID}" "${NEW_USER}"
  id -u "${NEW_USER}" >/dev/null 2>&1 || useradd -m -s /bin/bash -u "${DESIRED_UID}" -g "${DESIRED_GID}" "${NEW_USER}"
fi

usermod -s /bin/bash "${NEW_USER}"
mkdir -p "/home/${NEW_USER}"
chown -R "${NEW_USER}:${NEW_USER}" "/home/${NEW_USER}"
