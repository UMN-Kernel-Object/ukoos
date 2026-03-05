# Devices

Devices that have been detected are represented as a tree of `struct device` in memory.
(This should not be confused with the [Devicetree](https://en.wikipedia.org/wiki/Devicetree) standard, which is one of several ways that ukoOS populates this tree.)

This tree's root node is stored in a global (`root_device`, declared in `device.h`).
The main purpose of this tree is to act as the single owner for all devices, in a way that allows describing dependencies between a device and the controller for the bus that it's on.
For example, it allows representing that one cannot (safely) disable a PCIe port with a card plugged into it, without first disabling that card.

## Device classes

Devices provide functionality other than simply existing.
These bits of functionality can be lumped into "device classes."
Device classes are not a single construct in the code, but are a design pattern in the kernel.

Typically, a device class consists of three things: a vtable struct, a device struct, and a list of devices.

The vtable is simply a struct with methods for that kind of device.
An example might be:

```c
struct rgb_led_ops {
  int get_colors(struct rgb_led *this, u8 *out_r, u8 *out_g, u8 *out_b);
  int set_colors(struct rgb_led *this, u8 r, u8 g, u8 b);
  int get_relative_brightness(struct rgb_led *this, u32 *out_r,
                              u32 *out_g, u32 *out_b);
};
```

A device struct is a struct that can be linked into the list of devices.

```c
struct rgb_led {
  struct list_head list;
  const struct rgb_led_ops *ops;
  // any fields that make sense

  // maybe including a ptr to the _same_ device
  struct device *device;
};
```

The device list is just a global list head that can have instances of the device class pushed onto it.

```c
struct list_head rgb_leds;
```

## Device discovery and creation

Devices can be created and attached to the tree of devices at runtime, not just during boot.
There are a few interesting points to look at during boot, though.

1. Early in boot (as soon as the allocator is up), drivers register themselves with device classes they can detect devices on.

   For example, a hypothetical ACME LED driver might contain code like:

   ```c
   struct acme_led_regs {
     u8 r, g, b;
     u8 rsvd0;
     u8 flags, flags_set, flags_clr;
     u8 rsvd1;
   };

   struct acme_led {
     struct device device;
     struct rgb_led rgb_led;
     struct acme_led_regs *regs;
   };

   // Defined later in this documentation.
   static int acme_led_get_colors(struct rgb_led *rgb_led, u8 *out_r, u8 *out_g, u8 *out_b);

   static const struct rgb_led_ops acme_led_ops = {
     .get_colors = acme_led_get_colors,
     // ...and so on...
   };

   // Defined later in this documentation.
   static struct device *acme_led_enumerate_dt(struct devicetree_node *node);

   // I don't really know what enumerating with other device classes would look
   // like; this is just an example.
   static struct device *acme_led_enumerate_usb(/* ... */) { /* ... */ }

   DEFINE_INIT(INIT_REGISTER_DRIVERS) {
     devicetree_register("acme0142", acme_led_enumerate_dt);
     devicetree_register("acme,ledrgb", acme_led_enumerate_dt);
     usb_register(0x1234, 0x5678, acme_led_enumerate_usb);
   }
   ```

   Registering a callback for the driver just populates a table to be used in later steps.
   (In the future, this might even get moved to compile-time so the allocator isn't necessary and lookups can be accelerated with [gperf](https://www.gnu.org/software/gperf/).)

2. At some point later, those table entries get used, and the appropriate functions get called.

   The Devicetree, for example, can get traversed as soon as all the drivers are registered, since it's already been parsed as part of setting up the allocator.
   Buses like PCI or USB might wait until a later point to be traversed, or might even be traversed multiple times in order to handle hotplug.

   For the sake of our running example, let's say the Devicetree node looks like:

   ```dts
   rgb0@45670000 {
     compatible = "acme0142", "generic-rgb-led";
     reg = <0x00000000 0x45670000
            0x00000000 0x00001000>;
   }
   ```

   At some point during boot, the `devicetree_enumerate()` function will get called.
   This function will walk the tree and look on each node for a `compatible` property.
   If one is found, it'll call the callback (`acme_led_enumerate_dt` in thie case) and pass it the node.

3. When a callback is called, it creates the device object.

   `acme_led_enumerate_dt` might look something like:

   ```c
   static struct device *acme_led_enumerate_dt(struct devicetree_node *node) {
     struct acme_led *led = nullptr;
     paddr reg_addr;
     usize reg_size;

     // Grab the information we need to create the device from its Devicetree
     // node.
     if (!devicetree_reg(node, &reg_addr, &reg_size))
       goto fail;
     if (reg_size < sizeof(struct acme_led_regs));
       goto fail;

     // Allocate the device object.
     struct acme_led *led = alloc(sizeof(struct acme_led));
     if (!led)
       goto fail;

     // Initialize the device object.
     *led = (struct acme_led) {
       .device = DEVICE_INIT(led->device, "acme_led@{paddr}", reg_addr),
       .rgb_led = {
         .list = LIST_INIT(led->rgb_led.list),
         .ops = &acme_led_ops,
         .device = &led->device,
       },
       .regs = iomem_map(reg_addr, reg_size),
     };
     if (!led->device.name || !led->regs)
       goto fail;

     // Store the device in the appropriate global data structures. The device
     // will get stored in the tree of devices once it gets returned.
     list_push(&rgb_leds, &led->rgb_led.list);

     return led;

   fail:
     if (led) {
       free(led->device.name);
       iomem_unmap(led->regs, reg_size);
     }
     free(led);
     return nullptr;
   }
   ```

4. As various buses are discovered, their enumerate functions are called too.
   For example, after `devicetree_enumerate` returns, `pci_enumerate` might be called to find devices on the PCIe bus that was discovered in the Devicetree.
   Later, `usb_enumerate` might be called to find devices on USB ports discovered in the Devicetree or PCIe bus.
   This might be called repeatedly as devices are added and removed.

## Device methods

The above approach lets other modules that're oblivious to which drivers exist pass around `struct rgb_led *`s without worrying about their contents.

When these modules want to actually perform an operation, they call the functions in `rgb_led_ops`.
For example:

```c
int turn_off_blue(struct rgb_led *led) {
  u8 r, g;
  int err;

  err = led->ops->get_colors(led, &r, &g, nullptr);
  if (!err)
    return err;

  return led->ops->set_colors(led, r, g, 0);
}
```

If `led` is the device object created by our example `acme_led` driver, its `get_colors` function will be `acme_led_get_colors`.
This function converts the `struct rgb_led` to a `struct acme_led`, then does the appropriate IO.

```c
static int acme_led_get_colors(struct rgb_led *rgb_led, u8 *out_r, u8 *out_g, u8 *out_b) {
  struct acme_led *this;
  u8 r, g, b;

  this = container_of(rgb_led, struct acme_led, rgb_led);
  r = READ_ONCE(this->regs->r);
  g = READ_ONCE(this->regs->g);
  b = READ_ONCE(this->regs->b);

  if (out_r)
    *out_r = r;
  if (out_g)
    *out_g = g;
  if (out_b)
    *out_b = b;

  return 0;
}
```

The `container_of` macro converts from a pointer to a field of a struct into a pointer to the struct.
It lets us get the containing `struct acme_led`.

The `READ_ONCE` macro is equivalent to the same macro in Linux.
It performs a read that the compiler is not permitted to optimize away (hoist out of a loop, constant-fold, etc.).
This read is atomic, but does not synchronize with any other operation any more than a normal read does (i.e., there's no implicit fence).
