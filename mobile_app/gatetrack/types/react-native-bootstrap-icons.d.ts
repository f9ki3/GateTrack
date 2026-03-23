declare module "react-native-bootstrap-icons" {
  import { SvgProps } from "react-native-svg";
  export interface BootstrapIconProps extends SvgProps {
    size?: number | string;
    color?: string;
    fill?: string;
  }
  // Add all exported icons
  export const ShieldFill: React.FC<BootstrapIconProps>;
  export const ShieldLockFill: React.FC<BootstrapIconProps>;
  export const EnvelopeFill: React.FC<BootstrapIconProps>;
  export const KeyFill: React.FC<BootstrapIconProps>;
  export const EyeFill: React.FC<BootstrapIconProps>;
  export const EyeSlashFill: React.FC<BootstrapIconProps>;
  export const ArrowRight: React.FC<BootstrapIconProps>;
  export const GearFill: React.FC<BootstrapIconProps>;
  export const ClockHistoryFill: React.FC<BootstrapIconProps>;
  // Add more as used
  const _unused: never; // For type checking
}
