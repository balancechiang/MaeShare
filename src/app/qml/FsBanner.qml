// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Maemo-5 style translucent bottom notification pill.
import QtQuick 1.0

Rectangle {
    id: banner
    // 宽度按字符数估算，避免与内部 Text 宽度形成绑定环
    width: Math.min(740, Math.max(260, banner.text.length * 20 + 52))
    height: 56
    radius: 28
    smooth: true
    color: "#E6111317"
    border.color: "#3A5450"
    border.width: 1
    opacity: 0
    visible: opacity > 0
    z: 200
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 22

    property string text: ""

    function show() {
        banner.opacity = 1;
        hideTimer.restart();
    }

    Behavior on opacity {
        NumberAnimation { duration: 180 }
    }

    Text {
        id: bannerText
        anchors.centerIn: parent
        width: banner.width - 44
        text: banner.text
        font.pixelSize: 20
        color: "#FFFFFF"
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Timer {
        id: hideTimer
        interval: 4200
        onTriggered: banner.opacity = 0
    }

    MouseArea {
        anchors.fill: parent
        onClicked: banner.opacity = 0
    }
}
