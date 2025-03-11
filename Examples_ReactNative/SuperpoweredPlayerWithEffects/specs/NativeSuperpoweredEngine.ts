import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
    initSuperpowered(): void;
    togglePlayback(): void;
    enableFlanger(enable: boolean): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>(
  'NativeSuperpoweredEngine',
);
