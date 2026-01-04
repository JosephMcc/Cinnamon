const Applet = imports.ui.applet;
const Clutter = imports.gi.Clutter;
const Main = imports.ui.main;
const Gtk = imports.gi.Gtk;
const Gio = imports.gi.Gio;
const PopupMenu = imports.ui.popupMenu;
const St = imports.gi.St;
const Mainloop = imports.mainloop;
const Placeholder = imports.ui.placeholder;
const Urgency = imports.ui.messageTray.Urgency;
const MessageTray = imports.ui.messageTray;
const NotificationDestroyedReason = imports.ui.messageTray.NotificationDestroyedReason;
const Settings = imports.ui.settings;
const Gettext = imports.gettext.domain("cinnamon-applets");
const Util = imports.misc.util;

const PANEL_EDIT_MODE_KEY = "panel-edit-mode";

class CinnamonNotificationsApplet extends Applet.TextIconApplet {
    constructor(metadata, orientation, panel_height, instanceId) {
        super(orientation, panel_height, instanceId);

        this.setAllowedLayout(Applet.AllowedLayout.BOTH);

        // Settings
        this.settings = new Settings.AppletSettings(this, metadata.uuid, instanceId);
        this.settings.bind("ignoreTransientNotifications", "ignoreTransientNotifications");
        this.settings.bind("showEmptyTray", "showEmptyTray", this._showHideTray);
        this.settings.bind("keyOpen", "keyOpen", this._setKeybinding);
        this.settings.bind("keyClear", "keyClear", this._setKeybinding);
        this.settings.bind("showNotificationCount", "showNotificationCount", this.updateList);
        this.settings.bind("showNewestFirst", "showNewestFirst", this.updateList);
        this._setKeybinding();

        // Layout
        this._orientation = orientation;
        this.menuManager = new PopupMenu.PopupMenuManager(this);

        // Lists
        this.notifications = [];    // The list of notifications, in order from oldest to newest.

        // Events
        Main.messageTray.connect('notify-applet-update', this._notificationAdded.bind(this));
        this.panelEditModeHandler = global.settings.connect('changed::' + PANEL_EDIT_MODE_KEY, this._on_panel_edit_mode_changed.bind(this));

        // States
        this._blinking = false;
        this._blinkToggle = false;
    }

    _setKeybinding() {
        Main.keybindingManager.addXletHotKey(this, "notification-open", this.keyOpen, this._openMenu.bind(this));
        Main.keybindingManager.addXletHotKey(this, "notification-clear", this.keyClear, this._clearAll.bind(this));
    }

    on_applet_removed_from_panel () {
        Main.keybindingManager.removeXletHotKey(this, "notification-open");
        Main.keybindingManager.removeXletHotKey(this, "notification-clear");
        global.settings.disconnect(this.panelEditModeHandler);

        MessageTray.extensionsHandlingNotifications--;
        if (MessageTray.extensionsHandlingNotifications === 0) {
            this._clearAll();
        }
    }

    _openMenu() {
        this._updateTimestamp();
        this.menu.toggle();
    }

    _display() {
        // Always start the applet empty, void of any notifications.
        this.set_applet_icon_symbolic_name("empty-notif");
        this.set_applet_tooltip(_("Notifications"));

        // Setup the notification container.
        this._maincontainer = new St.BoxLayout({
            name: 'traycontainer',
            vertical: true,
        });
        this._notificationbin = new St.BoxLayout({vertical:true});

        const titleBox = new St.BoxLayout({ style_class: 'applet-title-box' });
        const label = new St.Label({
            style_class: 'title',
            text: _("Notifications"),
            y_align: Clutter.ActorAlign.CENTER,
        });
        titleBox.add_child(label);
        const spacer = new St.BoxLayout({ x_expand: true });
        titleBox.add_child(spacer);
        const icon = new St.Icon ({
            icon_name: 'xsi-preferences-symbolic',
            icon_type: St.IconType.SYMBOLIC,
        });
        const settingsButton = new St.Button({
            style_class: 'icon-button',
            can_focus: true,
        });
        settingsButton.child = icon;
        settingsButton.connect('clicked', () => {
            Util.spawnCommandLine("cinnamon-settings notifications");
            this.menu.close();
        });
        titleBox.add_child(settingsButton);
        this.menu.box.add_child(titleBox);

        this.placeHolder = new Placeholder.Placeholder({
            icon_name: 'xsi-notifications-disabled-symbolic',
            title: _("No Notifications"),
        });
        this.menu.box.add_child(this.placeHolder);

        this.clearBox = new St.BoxLayout({
            style_class: 'clear-notification-box',
            x_align: Clutter.ActorAlign.END,
        });
        const button = new St.Button({
            style_class: 'button',
            label: _("Clear"),
            can_focus: true,
        });
        button.connect('clicked', this._clearAll.bind(this));
        this.clearBox.add_child(button);

        this.menu.addActor(this._maincontainer);
        this.menu.addActor(this.clearBox);

        this.scrollview = new St.ScrollView({
            x_fill: true,
            y_fill: true,
            y_align: St.Align.START,
            style_class: "vfade",
        });
        this._maincontainer.add(this.scrollview);
        this.scrollview.add_actor(this._notificationbin);
        this.scrollview.set_policy(St.PolicyType.NEVER, St.PolicyType.AUTOMATIC);
        this.scrollview.set_clip_to_allocation(true);

        let vscroll = this.scrollview.get_vscroll_bar();
        vscroll.connect('scroll-start', () => {
            this.menu.passEvents = true;
        });
        vscroll.connect('scroll-stop', () => {
            this.menu.passEvents = false;
        });

        // Alternative tray icons.
        this._critIcon = new St.Icon({
            icon_name: 'critical-notif-symbolic',
            icon_type: St.IconType.SYMBOLIC,
            reactive: true,
            track_hover: true,
            style_class: 'system-status-icon',
        });
        this._altCritIcon = new St.Icon({
            icon_name: 'alt-critical-notif-symbolic',
            icon_type: St.IconType.SYMBOLIC,
            reactive: true,
            track_hover: true,
            style_class: 'system-status-icon',
        });

        this._on_panel_edit_mode_changed();
    }

    _notificationAdded (mtray, notification) { // Notification event handler.
        // Ignore transient notifications?
        if (this.ignoreTransientNotifications && notification.isTransient) {
            notification.destroy();
            return;
        }

        notification.actor.unparent();
        let existingIndex = this.notifications.indexOf(notification);
        if (existingIndex != -1) { // This notification is already listed.
            if (notification._destroyed) {
                this.notifications.splice(existingIndex, 1);
            } else {
                notification._inNotificationBin = true;
                global.reparentActor(notification.actor, this._notificationbin);
                notification._timeLabel.show();
            }
            this.updateList();
            return;
        } else if (notification._destroyed) {
            return;
        }
        // Add notification to list.
        notification._inNotificationBin = true;
        this.notifications.push(notification);
        // Steal the notification panel.
        this._notificationbin.add(notification.actor);
        notification.actor._parent_container = this._notificationbin;
        notification.actor.add_style_class_name('notification-applet-padding');
        // Register for destruction.
        notification.connect('scrolling-changed', (notif, scrolling) => {
            this.menu.passEvents = scrolling
        });
        notification.connect('destroy', () => {
            let i = this.notifications.indexOf(notification);
            if (i != -1)
                this.notifications.splice(i, 1);
            this.updateList();
        });
        notification._timeLabel.show();

        this.updateList();
    }

    updateList () {
        try {
            let count = this.notifications.length;
            if (count > 0) {    // There are notifications.
                this.actor.show();
                this.placeHolder.hide();
                this.clearBox.show();
                this.set_applet_label(count.toString());
                this._reorderNotifications();
                // Find max urgency and derive list icon.
                let maxUrgency = -1;
                for (let i = 0; i < count; i++) {
                    let curUrgency = this.notifications[i].urgency;
                    if (curUrgency > maxUrgency)
                        maxUrgency = curUrgency;
                }
                switch (maxUrgency) {
                    case Urgency.LOW:
                        this._blinking = false;
                        this.set_applet_icon_symbolic_name("low-notif");
                        break;
                    case Urgency.NORMAL:
                    case Urgency.HIGH:
                        this._blinking = false;
                        this.set_applet_icon_symbolic_name("normal-notif");
                        break;
                    case Urgency.CRITICAL:
                        if (!this._blinking) {
                            this._blinking = true;
                            this.criticalBlink();
                        }
                        break;
                }
            } else {    // There are no notifications.
                this._blinking = false;
                this.set_applet_label('');
                this.set_applet_icon_symbolic_name("empty-notif");
                this.placeHolder.show();
                this.clearBox.hide();
                if (!this.showEmptyTray) {
                    this.actor.hide();
                }
            }

            if (!this.showNotificationCount) {  // Don't show notification count
                this.set_applet_label('');
            }
            this._notificationbin.queue_relayout();
        }
        catch (e) {
            global.logError(e);
        }
    }

    _clearAll() {
        let count = this.notifications.length;
        if (count > 0) {
            for (let i = count-1; i >=0; i--) {
                this._notificationbin.remove_actor(this.notifications[i].actor);
                this.notifications[i].destroy(NotificationDestroyedReason.DISMISSED);
            }
        }
        this.notifications = [];
        this.updateList();
    }

    _reorderNotifications() {
        let orderedNotifications = this.notifications.slice();

        if (this.showNewestFirst) {
            orderedNotifications.reverse();
        }

        // Remove all children without destroying them.
        let children = this._notificationbin.get_children();
        for (let i = 0; i < children.length; i++) {
            this._notificationbin.remove_child(children[i]);
        }

        // Add them back in desired order.
        for (let i = 0; i < orderedNotifications.length; i++) {
            this._notificationbin.add_child(orderedNotifications[i].actor);
        }
    }

    _showHideTray() { // Show or hide the notification tray.
        if(!global.settings.get_boolean(PANEL_EDIT_MODE_KEY)) {
            if (this.notifications.length || this.showEmptyTray) {
                this.actor.show();
            } else {
                this.actor.hide();
            }
        }
    }

    _on_panel_edit_mode_changed () {
        if (global.settings.get_boolean(PANEL_EDIT_MODE_KEY)) {
            this.actor.show();
        } else {
            this.updateList();
        }
    }

    on_applet_added_to_panel() {
        this.on_orientation_changed(this._orientation);
        MessageTray.extensionsHandlingNotifications++;
    }

    on_orientation_changed (orientation) {
        this._orientation = orientation;

        if (this.menu) {
            this.menu.destroy();
        }
        this.menu = new Applet.AppletPopupMenu(this, orientation);
        this.menuManager.addMenu(this.menu);
        this.menu.setCustomStyleClass('notification-applet');
        this._display();
    }

    on_applet_clicked(event) {
        this._openMenu();
    }

    _updateTimestamp() {
        let len = this.notifications.length;
        if (len > 0) {
            for (let i = 0; i < len; i++) {
                let notification = this.notifications[i];
                let origTime = notification._timestamp;
                notification._timeLabel.clutter_text.set_markup(timeify(origTime));
            }
        }
    }

    criticalBlink () {
        if (!this._blinking)
            return;
        if (this._blinkToggle) {
            this._applet_icon_box.child = this._critIcon;
        } else {
            this._applet_icon_box.child = this._altCritIcon;
        }
        this._blinkToggle = !this._blinkToggle;
        Mainloop.timeout_add_seconds(1, this.criticalBlink.bind(this));
    }
}

function main(metadata, orientation, panel_height, instanceId) {
    return new CinnamonNotificationsApplet(metadata, orientation, panel_height, instanceId);
}

function stringify(count) {
    if (count === 0) {
        return _("No notifications");
    } else {
        return ngettext("%d notification", "%d notifications", count).format(count);
    }
}

function timeify(orig_time) {
    let settings = new Gio.Settings({schema_id: 'org.cinnamon.desktop.interface'});
    let use24h = settings.get_boolean('clock-use-24h');
    let now = new Date();
    let diff = Math.floor((now.getTime() - orig_time.getTime()) / 1000); // get diff in seconds
    let str;
    if (use24h) {
        str = orig_time.toLocaleFormat('%x, %T');
    } else {
        str = orig_time.toLocaleFormat('%x, %r');
    }
    switch (true) {
        case (diff <= 15): {
            str += " (" + _("just now") + ")";
            break;
        } case (diff > 15 && diff <= 59): {
            str += " (" + ngettext("%d second ago", "%d seconds ago", diff).format(diff) + ")";
            break;
        } case (diff > 59 && diff <= 3540): {
            let diffMinutes = Math.floor(diff / 60);
            str += " (" + ngettext("%d minute ago", "%d minutes ago", diffMinutes).format(diffMinutes) + ")";
            break;
        }
    }
    return str;
}
