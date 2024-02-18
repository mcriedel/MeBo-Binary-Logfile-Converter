import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls.Fusion

ApplicationWindow {
    id: root
    width: 540
    height: 570
    visible: true
    title: qsTr("MeBo Logfile Converter")

    palette.highlight: "#FF5722"

    property var input_files: ""
    property alias output_folder: output_folder_field.text
    property bool edited: true

    component StringListView: Rectangle {
        id: lv_holder
        property alias model: lv.model
        property bool editable: false

        color: palette.base

        ListView {
            id:lv
            anchors.fill: parent
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                implicitHeight: 12
                Rectangle {
                    anchors.fill: parent
                    visible: true
                    color: lv.currentIndex === index ? palette.highlight: "#FF000000"
                    Keys.onCancelPressed: {
                        if(lv.currentIndex>= 0){
                            lv.model.remove(lv.currentIndex)
                            lv.currentIndex = -1
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: lv.currentIndex = index
                    }
                }

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 5
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: palette.text
                }
            }
        }
    }

    component ModGroupBox: GroupBox {
        implicitWidth: parent.width
        background: Rectangle {
            width: parent.width
            height: parent.height
            color: "transparent"
            border.color: palette.placeholderText
            radius: 2
        }
    }

    component HorizontalSpacer: Item {
        implicitHeight: 10
        Layout.fillWidth: true
    }

    function clearProgress(){
        prog_bar.value = 0.
        conv.clearErrorStrings()
    }

    FileDialog {
        id: input_select
        fileMode: FileDialog.OpenFiles
        nameFilters: ["MeBo binary files (*.hlf)"]
        onAccepted: {
            var files = []
            for(var i = 0; i<selectedFiles.length; i++){
                files[i] = String(selectedFiles[i]).replace("file:///", "")
            }

            root.input_files = files
            root.edited = true
            clearProgress()
        }
    }

    FolderDialog {
        id: output_select
        onAccepted: {
            var folder = String(selectedFolder)
            folder = folder.replace("file:///", "")
            root.output_folder = folder
            root.edited = true
            clearProgress()
        }
    }

    ColumnLayout {
        id: main_layout
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10
        Layout.alignment: Qt.AlignHCenter

        ModGroupBox {
            title: qsTr("Input files")
            Layout.alignment: Qt.AlignHCenter

            ColumnLayout {
                anchors.fill: parent
                spacing: 5
                Layout.alignment: Qt.AlignHCenter

                StringListView {
                    implicitHeight: 100
                    Layout.fillWidth: true
                    model: root.input_files
                    Layout.alignment: Qt.AlignHCenter
                    editable:true
                }

                Button {
                    id: select_files
                    text: "Select binary files"
                    onClicked: input_select.open()
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        ModGroupBox {
            title: qsTr("Output directory")
            Layout.alignment: Qt.AlignHCenter

            RowLayout {
                anchors.fill: parent
                spacing: 5

                TextField {
                    id: output_folder_field
                    Layout.fillWidth: true
                    leftPadding: 5
                    placeholderText: qsTr("Output directoy")
                    onAccepted: {
                        focus = false
                        clearProgress()
                    }
                    text: ""
                }

                Button {
                    id: select_target
                    text: "Select output directory"
                    onClicked: output_select.open()
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        ModGroupBox {
            title: qsTr("Options")
            Layout.alignment: Qt.AlignHCenter

            RowLayout {
                anchors.fill: parent
                Layout.alignment: Qt.AlignHCenter

                HorizontalSpacer{}

                Item {
                    implicitHeight: sep_text.implicitHeight > sep_input.implicitHeight ?sep_text.implicitHeight : sep_input.implicitHeight
                    implicitWidth: sep_text.implicitWidth + sep_input.implicitWidth
                    Layout.alignment: Qt.AlignHCenter
                    Text{
                        id: sep_text
                        color: palette.text
                        verticalAlignment: Text.AlignVCenter
                        text: "Separator: "
                    }

                    TextField {
                        id: sep_input
                        implicitWidth: 40
                        horizontalAlignment: Text.AlignHCenter
                        anchors.left: sep_text.right
                        leftPadding: 5
                        placeholderText: qsTr("separator")
                        onAccepted: {
                            focus = false
                            clearProgress()
                        }
                        text: conv.separator
                    }
                }

                HorizontalSpacer{}

                Switch {
                    text: qsTr("Convert to euler angles")
                    checked: conv.convert_to_euler
                    onCheckedChanged: conv.convert_to_euler = checked
                }

                HorizontalSpacer{}
            }
        }

        ModGroupBox {
            title: "Progress"
            Layout.alignment: Qt.AlignHCenter

            ColumnLayout {
                anchors.fill: parent

                Connections {
                    target: conv
                    function onFinishedFilesChanged(){
                        if(conv.finishedFiles === 0){
                            prog_bar.value = 0.
                        }
                        else {
                            prog_bar.value = (conv.finishedFiles).toFixed(2)/(conv.maxFiles).toFixed(2)
                        }
                    }
                }

                ProgressBar {
                    id: prog_bar
                    value: 0.
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                }

                StringListView {
                    implicitHeight: 100
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    model: conv.errorStrings
                }
            }
        }

        Button {
            id: convert
            text: "Convert"
            onClicked: conv.convert_func(root.input_files, root.output_folder)
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
