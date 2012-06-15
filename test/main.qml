import QtQuick 1.1
import com.nokia.meego 1.0
import MeeGo.Connman 0.2
import "/usr/lib/qt4/imports/Connman/js/mustache.js" as M

PageStack {
    id: wifiPageStack
    anchors { fill: parent }
    Component.onCompleted: {
        wifiPageStack.push(initialPage)
    }

    function handleInput(input) {
        console.log("About to handle input "+ input);
        for (var key in input) {
            console.log(key + "-> " + input[key]);
        }
        networkingModel.sendUserReply(input);
        wifiPageStack.pop()
        scanTimer.running = true;
    }

    Timer {
        id: scanTimer
        interval: 25000
        running: false
        repeat: true
        triggeredOnStart: true
        onTriggered: networkingModel.requestScan();
    }

    NetworkingModel {
        id: networkingModel
        property string form_tpl: "
            import QtQuick 1.1
            import com.nokia.meego 1.0
            Item {
                id: form
                signal send (variant input)
                anchors { fill: parent }
                Column {
                    anchors { fill: parent }
                    {{#fields}}
                    Text {
                        text: '{{name}} ({{type}} {{requirement}})'
                    }
                    TextField {
                        id: {{id}}
                        placeholderText: 'enter {{name}}'
                    }
                    {{/fields}}
                    Button {
                        text: 'Connect'
                        onClicked: {
                            console.log('clicked Connect');
                            var inputs = {};
                            {{#fields}}
                            if ({{id}}.text) {
                                inputs['{{name}}'] = {{id}}.text;
                            }
                            {{/fields}}
                            form.send.connect(handleInput); form.send(inputs);
                        }
                    }
                }
            }
        "
        onTechnologiesChanged: {
            wifiSwitch.checked = networkingModel.wifiPowered;
            wifiPageStack.replace(mainPage);
            scanTimer.running = networkingModel.wifiPowered;
        }
        onWifiPoweredChanged: {
            wifiSwitch.checked = networkingModel.wifiPowered;
            scanTimer.running = networkingModel.wifiPowered;
        }

        onUserInputRequested: {
            scanTimer.running = false;
            scanTimer.triggeredOnStart = false;
            console.log("USER INPUT REQUESTED");
            var view = {
                "fields": []
            };
            for (var key in fields) {
                view.fields.push({
                    "name": key,
                    "id": key.toLowerCase(),
                    "type": fields[key]["Type"],
                    "requirement": fields[key]["Requirement"]
                });
                console.log(key + ":");
                for (var inkey in fields[key]) {
                    console.log("    " + inkey + ": " + fields[key][inkey]);
                }
            }
            var output = M.Mustache.render(form_tpl, view);
            console.log("output:" + output);
            var form = Qt.createQmlObject(output, dynFields, "dynamicForm1");
            wifiPageStack.push(networkPage);
        }
    }

    Page {
        id: initialPage

        Text {
            text: "Hello world"
            color: "red"
            font.pointSize: 24
        }
    }

    Component {
        id: networkRow

        Rectangle {
            height: 40
            color: "yellow"
            anchors { left: parent.left; right: parent.right }

            Text {
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 20 }
                text: modelData.name
            }
            Text {
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                text: modelData.favorite ? modelData.state + " (favorite)" : modelData.state
            }
            Text {
                anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 40 }
                text: modelData.security.join() + " " + modelData.strength
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("clicked " + modelData.name);
                    modelData.requestConnect();
                    networkName.text = modelData.name;
                }
            }
        }
    }

    Page {
        id: mainPage

        Column {
            spacing: 10
            anchors { fill: parent }
            Rectangle {
                color: "green"
                anchors { left: parent.left; right: parent.right }
                height: 60
                Text {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 20 }
                    text: "WiFi status"
                    color: "white"
                    font.pointSize: 16
                }
                Switch {
                    id: wifiSwitch
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 40 }
                    checked: networkingModel.wifiPowered
                    onCheckedChanged: {
                        networkingModel.wifiPowered = wifiSwitch.checked
                    }
                }
            }
            ListView {
                anchors { left: parent.left; right: parent.right }
                clip: true
                height: 700
                spacing: 5
                model: networkingModel.networks
                delegate: networkRow
            }
        }
    }

    Page {
        id: networkPage

        Column {
            spacing: 10
            anchors { fill: parent }
            Rectangle {
                color: "green"
                anchors { left: parent.left; right: parent.right }
                height: 60

                Text {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 20 }
                    text: "Wireless Network Details"
                    color: "white"
                    font.pointSize: 16
                }
            }
            Text {
                text: "Network:"
                color: "green"
            }
            Text {
                id: networkName
                color: "green"
            }
            Rectangle {
                anchors { left: parent.left; right: parent.right }
                height: 200
                id: dynFields
            }
        }
    }
}
