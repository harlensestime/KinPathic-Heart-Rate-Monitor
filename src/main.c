#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/hrs.h>

/* ---------------- Thread settings ---------------- */
#define STACKSIZE 1024
#define PRIORITY 7

static struct bt_conn *default_conn;

/* ---------------- Advertising data ---------------- */
/* 0x180D = Heart Rate Service UUID */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0d, 0x18),
};

/* New style advertising parameters */
static const struct bt_le_adv_param adv_param = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
    .secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .peer = NULL,
};

/* ---------------- Connection Callbacks ---------------- */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
    } else {
        default_conn = bt_conn_ref(conn);
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);

    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* ---------------- Heart Rate Thread ---------------- */
void hrm_thread(void)
{
    uint8_t fake_bpm = 72;

    while (1) {
        k_sleep(K_SECONDS(2));

        bt_hrs_notify(fake_bpm);
        printk("Heart Rate: %u bpm\n", fake_bpm);
    }
}

K_THREAD_DEFINE(hrm_id, STACKSIZE, hrm_thread, NULL, NULL, NULL,
                PRIORITY, 0, 0);

/* ---------------- Main Entry ---------------- */
int main(void)
{
    int err;

    printk("Starting Kinpathic Heart Rate Monitor\n");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }
    printk("Bluetooth initialized\n");

    /* Start advertising with HRS UUID */
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
    } else {
        printk("Advertising successfully started\n");
    }

    return 0;
}
