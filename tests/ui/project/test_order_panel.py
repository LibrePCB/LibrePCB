#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test ordering projects with the order panel
"""


def test_success(librepcb):
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        # Open the order panel
        order_btn = app.get("SideBar::order-pcb-btn")
        order_btn.wait_for(enabled=True)
        order_btn.click()

        # Wait until the API info request succeeded
        upload_btn = app.get("OrderPanel::upload-btn")
        upload_btn.wait_for(enabled=True, timeout=10.0)
        assert upload_btn.label == "Upload Project"

        # Disable auto-open in the browser to avoid possibly interrupting
        # the test
        app.get("OrderPanel::open-browser-sw").set_checked(False)

        # Start upload and wait until it succeeds
        upload_btn.click()
        upload_btn.wait_for(enabled=False, timeout=1.0)
        status_txt = app.get("OrderPanel::status-txt")
        status_txt.wait_for(
            value="Success! Please continue in the web browser:", timeout=10.0
        )

        # After a successful upload the button is disabled and the retry delay
        # countdown is shown in the button text.
        upload_btn.wait_for_item(
            lambda btn: btn.label.startswith("Upload Project ("), timeout=10.0
        )
        upload_btn.wait_for(enabled=False, timeout=1.0)


def test_api_error(librepcb, api_server):
    # Let the project upload fail with an HTTP 500 error
    api_server.set_response("/api/v1/order/upload", status=500)

    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        # Open the order panel
        order_btn = app.get("SideBar::order-pcb-btn")
        order_btn.wait_for(enabled=True)
        order_btn.click()

        # Wait until the API info request succeeded
        upload_btn = app.get("OrderPanel::upload-btn")
        upload_btn.wait_for(enabled=True, timeout=10.0)
        assert upload_btn.label == "Upload Project"

        # Start upload and check the error
        upload_btn.click()
        upload_btn.wait_for(enabled=False, timeout=1.0)
        upload_btn.wait_for_item(
            lambda btn: btn.label.startswith("Upload Project ("), timeout=10.0
        )
        app.get("OrderPanel::status-txt").wait_for_item(
            lambda txt: txt.value.startswith("Error transferring")
        )
        app.get("OrderPanel::upload-url-btn ?").wait(0)
