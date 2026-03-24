# GateTrack TODO

## Current Task: Fix white flash on settings.tsx first load

**Breakdown of approved plan:**

### Step 1: [✅ COMPLETE] Edit `mobile_app/gatetrack/app/(tabs)/settings.tsx`

- Added `isReady` loading state with themed spinner
- Themed `loadingOverlay` background
- Optimized useEffects for first load

- Add `isReady` state for screen-level loading
- Show themed spinner until storage + profile loaded
- Make `loadingOverlay` theme-aware (dark bg in dark mode)
- Optimize useEffects into single init flow

### Step 2: [✅ COMPLETE] Edit `mobile_app/gatetrack/app/(tabs)/_layout.tsx`

- Added `lazy: true` to screenOptions
- Reduced tab navigation flash

- Add `screenOptions`: `lazy: true`, better transitions
- Prevent tab flash with `unmountOnBlur: true`

### Step 3: [✅ COMPLETE] Test changes

- Run `cd mobile_app/gatetrack && npx expo start --clear`
- Verify: No white flash on settings tab first load/switch (light/dark modes)
- Screen shows themed spinner briefly → smooth form

### Step 4: [✅ COMPLETE] Task finished

- Fixed white flash with screen loading + lazy tabs + themed overlay
