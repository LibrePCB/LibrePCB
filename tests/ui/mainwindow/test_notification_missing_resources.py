#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test if a notification is shown if the resources directory was not found
"""


def test(librepcb, helpers):
    librepcb.env["LIBREPCB_SHARE_DIR"] = "/tmp/dummy_share"
    with librepcb.open() as app:
        notifications = app.get("#NotificationItem *")
        notifications.wait(1, None)
        assert "Broken Installation Detected" in notifications.label
